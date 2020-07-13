#include <types.h>
#include <yaos/module.h>
#include <yaos/sysparam.h>
#include <yaos/yaoscall.h>
#include <yaos/percpu.h>
#include <yaos/init.h>
#include <yaoscall/malloc.h>
#include <yaos/errno.h>
#include <yaos/printk.h>
#include <yaos/lua_module.h>
#include <yaos/time.h>
#include "util.h"
#include "common.h"
#include "lwip/init.h"
#include "lwip/pbuf.h"
#if 0
#define DEBUG_PRINT printk
#define DEBUG_CALLSTACK() show_call_stack(4)
#else
#define DEBUG_PRINT inline_printk
#define DEBUG_CALLSTACK()
#endif
#define PERFORMANCE_TEST 0

extern struct pbuf * get_req_buf(struct http_state *hs);
struct ngx_lua_output_cleanup_s{
#if !CLEANUP_POOL
    http_cleanup_t clean;
#endif
    lua_State * L;
    int	ref;    
};
typedef struct ngx_lua_output_cleanup_s ngx_lua_output_cleanup_t;
static void ngx_lua_output_cleanup(void *data)
{
    ngx_lua_output_cleanup_t *clean = (ngx_lua_output_cleanup_t  *)data;
    //lua_rawgeti(clean->L, LUA_REGISTRYINDEX, clean->ref);
    //size_t len;
    //uchar *p = (uchar *) luaL_checklstring(clean->L, -1, &len);
    //ssize_t vga_write(void *p, size_t s);
    //vga_write(p,len);
    //lua_pop(clean->L, 1);
    luaL_unref(clean->L, LUA_REGISTRYINDEX, clean->ref);
    //DEBUG_PRINT("ngx_lua_output_cleanup:%d\n",clean->ref);

}
static void ngx_lua_ctx_cleanup(void *data)
{
   http_lua_ctx_t  *ctx = (http_lua_ctx_t  *)data; 
   lua_State *L;
   http_lua_co_ctx_t *coctx;
   if (ctx == NULL) return;
   L = ctx->L;
   coctx = &ctx->entry_co_ctx;
   if (coctx->co_ref != LUA_NOREF) {
       DEBUG_PRINT("ngx_lua_ctx_cleanup:ref:%d,ctx:%lx,type:%d,co:%lx,co_ctx:%lx\n",coctx->co_ref, ctx, lua_type(L, -1),ctx->cur_co_ctx->co, ctx->cur_co_ctx);
       ASSERT_PRINT((ulong)ctx->cur_co_ctx->co>0x10000,"co:%lx,ctx:%lx,co_ctx:%lx\n",ctx->cur_co_ctx->co,ctx,ctx->cur_co_ctx);
       http_lua_delete_thread(L, ctx->cur_co_ctx->co, coctx->co_ref);
       coctx->co_ref = LUA_NOREF;
       
   }
#if !CLEANUP_POOL
   yaos_mfree(ctx);
#endif
}
static int ngx_exit(lua_State *L)
{
    http_request_t *r = http_lua_get_req(L);
    if (r == NULL) {
        return luaL_error(L, "request object not found");
    }
    return 0; 
}
static int ngx_flush(lua_State *L)
{
    http_request_t *r = http_lua_get_req(L);
    if (r == NULL) {
        return luaL_error(L, "request object not found");
    }
    http_lua_ctx_t  *ctx = http_get_module_ctx(r, HTTP_LUA_MODULE);
    if (ctx == NULL) {
        return luaL_error(L, "ngx.flush no request ctx found");
    }

    http_lua_flush_output(r,ctx);
    return 0;
 
}

static int ngx_lua_say(lua_State *L)
{
    http_request_t *r = http_lua_get_req(L);
    if (r == NULL) {
        return luaL_error(L, "request object not found");
    }
    int argc = lua_gettop(L);
    if (argc != 1) return luaL_error(L, "ngx.say expecting one argument");
    http_lua_ctx_t  *ctx = http_get_module_ctx(r, HTTP_LUA_MODULE);
    if (ctx == NULL) {
        return luaL_error(L, "ngx.say no request ctx found");
    }
    uchar *p;
    size_t len;
    p = (uchar *) luaL_checklstring(L, 1, &len);
    httpd_chain_t *pout = httpd_chain_get_free_buf(r->pool, &ctx->free_buf);
    if (pout == NULL) return 0;
    pout->buf->start = pout->buf->pos = pout->buf->last = p;
    pout->buf->end = p + len;
    int err = http_lua_send_chain_link(r, ctx, pout);
    //err_t http_send_content(struct http_state *hs, char *p, size_t len);
    //err_t err = http_send_content(r->hs, p, len);
#if CLEANUP_POOL
    httpd_pool_cleanup_t *pclean = httpd_pool_cleanup_add(r->pool,
       sizeof(ngx_lua_output_cleanup_t));
#else
    ngx_lua_output_cleanup_t *pclean = (ngx_lua_output_cleanup_t *)yaos_malloc(sizeof(ngx_lua_output_cleanup_t));
#endif
    if (!pclean) return 0;
    int  ref = luaL_ref(L, LUA_REGISTRYINDEX);
#if CLEANUP_POOL
    pclean->handler = ngx_lua_output_cleanup;
    ngx_lua_output_cleanup_t * data = pclean->data;
    data->L = L;
    data->ref = ref;
#else
    pclean->clean.data = pclean;
    pclean->clean.handler = ngx_lua_output_cleanup;
    pclean->L = L;
    pclean->ref = ref;
    http_add_cleanup(r, &pclean->clean); 
#endif
    DEBUG_PRINT("http_send_content:%d,ref:%d\n",err,ref);
    return 0;

}
static int ngx_lua_get_req(lua_State *L)
{
    http_request_t *r = http_lua_get_req(L);
    if (r == NULL) {
        return luaL_error(L, "request object not found");
    }

    struct pbuf * p = get_req_buf(r->hs);
    if (!p) {
       lua_pushnil(L);
       return 1;

    }
    DEBUG_PRINT("pbuf:%lx,total:%d,len:%d\n",p,p->tot_len, p->len);

    if (p->next == NULL) {
        DEBUG_PRINT("p->payload:%lx,len:%d\n",p->payload, p->len);
        lua_pushlstring(L, (char *) p->payload, p->len);
        return 1;

    }
    int nr = 0;
    while (p) {
        lua_pushlstring(L, (char *) p->payload, p->len);
        nr++;
        p = p->next;

    }
    return nr;
}
int ngx_lua_inject_content(lua_State *L)
{
    lua_pushcfunction(L, ngx_lua_get_req);
    lua_setfield(L, -2, "get_req");
    lua_pushcfunction(L, ngx_lua_say);
    lua_setfield(L, -2, "say");
    lua_pushcfunction(L, ngx_exit);
    lua_setfield(L, -2, "exit");
    lua_pushcfunction(L, ngx_flush);
    lua_setfield(L, -2, "flush");
    return LUA_MOD_ERR_OK;
}

int http_lua_content_by_file(const char *name, lua_State *L, http_request_t *r)
{
    int co_ref;
    lua_State *co;
#if PERFORMANCE_TEST
    ulong start = hrt_uptime();
#endif
#if CLEANUP_POOL
    httpd_pool_cleanup_t *pclean = httpd_pool_cleanup_add(r->pool,0);
#else
    http_cleanup_t *pclean = (http_cleanup_t *)yaos_malloc(sizeof(http_cleanup_t));
#endif
    if (!pclean) return ENOMEM;

    http_lua_ctx_t  *ctx = http_get_module_ctx(r, HTTP_LUA_MODULE); 
    if (ctx == NULL) {
       ctx = http_lua_create_ctx(r);
       if (ctx == NULL) {
#if !CLEANUP_POOL
           yaos_mfree(pclean);
#endif
           return ENOMEM;
       }
    } else {
       http_lua_reset_ctx(r, L, ctx);
    }
    if (load_lua_file(L, (char *)name) != 0) {
       yaos_mfree(pclean);
       ctx->cur_co_ctx = NULL;
       ctx->entry_co_ctx.co_ref = LUA_NOREF;
       ngx_lua_ctx_cleanup(ctx);
       return ESRCH;
    }
     /*  {{{ new coroutine to handle request */
    co = http_lua_new_thread(L, &co_ref);
    DEBUG_PRINT("co:%lx\n",co);
    if ( co == NULL ) {
#if !CLEANUP_POOL
        yaos_mfree(pclean);
#endif
        ngx_lua_ctx_cleanup(ctx);
        return ENOMEM;
    }
    pclean->data = ctx;
    pclean->handler = ngx_lua_ctx_cleanup;
#if !CLEANUP_POOL
    http_add_cleanup(r, pclean);
#endif
    lua_xmove(L, co, 1);
    DEBUG_PRINT("co top type:%d\n",lua_type(co, -1));
    http_lua_get_globals_table(co);
    lua_setfenv(co, -2);
    http_lua_set_req(co, r);
    ctx->cur_co_ctx = &ctx->entry_co_ctx;
    ctx->cur_co_ctx->co = co;
    ctx->cur_co_ctx->co_ref = co_ref;
    ctx->L = L;
    int rc = http_lua_run_thread(L, r, ctx, 0);
    if (rc != 0) {
        rc = http_lua_run_thread(L, r, ctx, 0);
        printk("try again rc:%d\n",rc);
    }
#if PERFORMANCE_TEST
    ulong period = hrt_uptime() - start;
    //printk("content_by_lua:period:%d,rc:%d,L:%lx,r:%lx\n",period,rc,L,r);
    printk("%d\n",period);
#endif
    return rc;
}
