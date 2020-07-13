#include <types.h>
#include <yaos/module.h>
#include <yaos/sysparam.h>
#include <yaos/yaoscall.h>
#include <yaos/percpu.h>
#include <yaos/init.h>

#include <yaos/errno.h>
#include <yaos/printk.h>
#include <yaos/lua_module.h>
#include <yaoscall/malloc.h>
#include "util.h"
#include "../../httpd/httpd_request.h"
#if 0
#define DEBUG_PRINT printk
#define DEBUG_CALLSTACK() show_call_stack(4)
#else
#define DEBUG_PRINT inline_printk
#define DEBUG_CALLSTACK()
#endif
#ifndef CLEANUP_POOL
#error "Define CLEANUP_POOL FIRST"
#endif
char lightu_coroutines_key;
struct thread_pool{
    struct thread_pool *pnext;
    lua_State * co;
    int ref;     
};
static DEFINE_PER_CPU(struct thread_pool *, thread_pool_head);
static DEFINE_PER_CPU(struct thread_pool *, thread_pool_free);
static void
http_lua_alloc_thread_pools()
{
    struct thread_pool * p = yaos_malloc(sizeof(struct thread_pool)*64);
    struct thread_pool *ptr = p;
    if (!p) return;
    for (int i=0;i<64;i++){
       ptr->pnext=(ptr+1); 
       ptr = ptr+1;
    }
    ptr->pnext = this_cpu_read(thread_pool_free);
    this_cpu_write(thread_pool_free, p);
}
__used static void
http_lua_thread_pool_free(lua_State *co, int ref)
{
    struct thread_pool * p = this_cpu_read(thread_pool_free);
    if(!p) http_lua_alloc_thread_pools();
    p = this_cpu_read(thread_pool_free);
    ASSERT(p);
    p->ref = ref;
    p->co  = co;
    p->pnext = this_cpu_read(thread_pool_head);
    this_cpu_write(thread_pool_head, p);
    DEBUG_PRINT("free pool:%lx,%lx,%d\n",p,co,ref);
}
void 
http_lua_delete_thread(lua_State *L, lua_State *co, int ref)
{
#if  LUA_THREAD_POOL
    http_lua_thread_pool_free(co, ref);
    lua_settop(co, 0);
#else
    lua_pushlightuserdata(L, http_lua_coroutines_key);
    lua_rawget(L, LUA_REGISTRYINDEX);
    lua_settop(co, 0);
    luaL_unref(L, -1, ref);
    lua_pop(L, 1); 
    
#endif    

}
static void
http_lua_init_registry(lua_State *L)
{

    /* {{{ register a table to anchor lua coroutines reliably:
     * {([int]ref) = [cort]} */
    lua_pushlightuserdata(L, http_lua_coroutines_key);
    lua_createtable(L, 0, 32 /* nrec */);
    lua_rawset(L, LUA_REGISTRYINDEX);
    /* }}} */

    /* create the registry entry for the Lua request ctx data table */
    lua_pushliteral(L, http_lua_ctx_tables_key);
    lua_createtable(L, 0, 32 /* nrec */);
    lua_rawset(L, LUA_REGISTRYINDEX);

}
static void
http_lua_init_globals(lua_State *L)
{

//    http_lua_inject_ngx_api(L);
}

void 
http_lua_init_vm(lua_State *L)
{
    http_lua_init_registry(L);
    http_lua_init_globals(L);
}
void http_lua_thread_cleanup()
{
 
}
lua_State *
http_lua_new_thread(lua_State *L, int *ref)
{
#if LUA_THREAD_POOL
    struct thread_pool * p = this_cpu_read(thread_pool_head);
    if(p) {
        this_cpu_write(thread_pool_head, p->pnext);
        *ref = p->ref;
        p->pnext = this_cpu_read(thread_pool_free);
        this_cpu_write(thread_pool_free, p);
        DEBUG_PRINT("use pool :%lx,%lx,%d\n",p,p->co,p->ref);
        return p->co;
    }
#endif
    int base = lua_gettop(L);
    lua_State *co;
    lua_pushlightuserdata(L, http_lua_coroutines_key); 
    lua_rawget(L, LUA_REGISTRYINDEX);

    co = lua_newthread(L);
    *ref  = luaL_ref(L, -2);
    if (*ref == LUA_NOREF) {
        lua_settop(L, base);  /* restore main thread stack */
        return NULL;
    }
    //printk("co:%lx,co->status:%d\n",co,lua_status(co));
    lua_settop(L, base);
    return co;
}

static inline void
http_lua_init_ctx(http_request_t *r, http_lua_ctx_t *ctx)
{
    memset(ctx, 0, sizeof(http_lua_ctx_t));
    ctx->ctx_ref = LUA_NOREF;
    ctx->entry_co_ctx.co_ref = LUA_NOREF;
    ctx->request = r;
}


http_lua_ctx_t *
http_lua_create_ctx(http_request_t *r)
{
    http_lua_ctx_t          *ctx;
#if CLEANUP_POOL
    ctx = httpd_palloc(r->pool, sizeof(http_lua_ctx_t));
    DEBUG_PRINT("http_lua_create_ctx p:r:%lx,ctx：%lx\n",r, ctx);

#else
    ctx = yaos_malloc(sizeof(http_lua_ctx_t));
    DEBUG_PRINT("http_lua_create_ctx m:r:%lx,ctx：%lx\n",r, ctx);

#endif
    if (ctx == NULL) {
        return NULL;
    }
    http_lua_init_ctx(r, ctx);
    http_set_ctx(r, ctx, HTTP_LUA_MODULE);
    return ctx;
}
static void
http_lua_finalize_threads(http_request_t *r,
    http_lua_ctx_t *ctx, lua_State *L)
{
}
void
http_lua_reset_ctx(http_request_t *r, lua_State *L,
    http_lua_ctx_t *ctx)
{

    http_lua_finalize_threads(r, ctx, L);

#if 0
    if (ctx->user_co_ctx) {
        /* no way to destroy a list but clean up the whole pool */
        ctx->user_co_ctx = NULL;
    }
#endif

    memset(&ctx->entry_co_ctx, 0, sizeof(http_lua_co_ctx_t));

    ctx->entry_co_ctx.co_ref = LUA_NOREF;

    ctx->entered_rewrite_phase = 0;
    ctx->entered_access_phase = 0;
    ctx->entered_content_phase = 0;

    ctx->exit_code = 0;
    ctx->exited = 0;


    ctx->co_op = 0;
}
int
http_lua_atpanic(lua_State *L)
{
    u_char                  *s = NULL;
    size_t                   len = 0;

    if (lua_type(L, -1) == LUA_TSTRING) {
        s = (u_char *) lua_tolstring(L, -1, &len);
    }

    if (s == NULL) {
        s = (u_char *) "unknown reason";
        len = sizeof("unknown reason") - 1;
    }
    printk("lua atpanic: Lua VM crashed, reason: %*s", len, s);
    return 0;
}
/*
 * description:
 *  run a Lua coroutine specified by ctx->cur_co_ctx->co
 * return value:
 *  NGX_AGAIN:      I/O interruption: r->main->count intact
 *  NGX_DONE:       I/O interruption: r->main->count already incremented by 1
 *  NGX_ERROR:      error
 *  >= 200          HTTP status code
 */
int
http_lua_run_thread(lua_State *L, http_request_t *r,
    http_lua_ctx_t *ctx, volatile int nrets)
{
    http_lua_co_ctx_t   *orig_coctx = ctx->cur_co_ctx;
    int rv;
    lua_atpanic(L, http_lua_atpanic);
    lua_atpanic(orig_coctx->co, http_lua_atpanic);

    rv = lua_resume (orig_coctx->co, nrets);
    //printk("rv:%d\n",rv);
    return rv;
}

int http_lua_send_chain_link(http_request_t *r, http_lua_ctx_t *ctx, httpd_chain_t *in)
{
    DEBUG_PRINT("http_lua_send_chain_link, r:%lx,ctx:%lx,in:%lx,ctx->out:%lx\n",r,ctx,in,ctx->out);

    httpd_chain_t *cl;
    httpd_chain_t **ll;
    for (cl = ctx->out, ll = &ctx->out; cl; cl = cl->next) {
        ll = &cl->next;
    }

    *ll = in;
    DEBUG_PRINT("http_lua_send_chain_link done: r:%lx,ctx:%lx,in:%lx,ctx->out:%lx\n",r,ctx,in,ctx->out);
    return 0;
}
static inline int http_lua_output_filter(http_request_t *r, httpd_chain_t *in)
{
    int ret = httpd_output_filter(r,in);
    return ret;

}
int http_lua_flush_output(http_request_t * r, http_lua_ctx_t *ctx)
{
    if (ctx->out) return http_lua_output_filter(r, ctx->out);
    return 0;
}
static __init int init_util_call(bool isbp)
{
    this_cpu_write(thread_pool_free, NULL);
    this_cpu_write(thread_pool_head, NULL);
    http_lua_alloc_thread_pools();
    return 0;
}

core_initcall(init_util_call);

