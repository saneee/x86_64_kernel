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
char lightu_coroutines_key;

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

lua_State *
http_lua_new_thread(lua_State *L, int *ref)
{
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

    ctx = yaos_malloc(sizeof(http_lua_ctx_t));
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
    rv = lua_resume (orig_coctx->co, nrets);
    printk("rv:%d\n",rv);
    return rv;
}

