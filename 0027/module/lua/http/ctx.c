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
int
http_lua_ngx_set_ctx_helper(lua_State *L, http_request_t *r,
    http_lua_ctx_t *ctx, int index);
static int http_lua_ngx_ctx_add_cleanup(http_request_t *r,
    int ref);

struct ngx_lua_ctx_cleanup_s{
#if !CLEANUP_POOL
    http_cleanup_t clean;
#endif
    lua_State * L;
    int ref;
};
typedef struct ngx_lua_ctx_cleanup_s ngx_lua_ctx_cleanup_t;
static void ngx_lua_ctx_cleanup(void *data)
{
    ngx_lua_ctx_cleanup_t *clean = (ngx_lua_ctx_cleanup_t  *)data;
    lua_State *L = clean->L;
    lua_pushliteral(L, http_lua_ctx_tables_key);
    lua_rawget(L, LUA_REGISTRYINDEX);
    luaL_unref(L, -1, clean->ref);

}

int
http_lua_ngx_get_ctx(lua_State *L)
{
    http_request_t          *r;
    http_lua_ctx_t          *ctx;

    r = http_lua_get_req(L);
    if (r == NULL) {
        return luaL_error(L, "no request found");
    }

    ctx = http_get_module_ctx(r, HTTP_LUA_MODULE);
    if (ctx == NULL) {
        return luaL_error(L, "no request ctx found");
    }
    //printk("ctx->ctx_ref:%x,%x %s\n", ctx->ctx_ref, LUA_NOREF,http_lua_ctx_tables_key);
    if (ctx->ctx_ref == LUA_NOREF) {
//int top = lua_gettop(L);
        lua_pushliteral(L, http_lua_ctx_tables_key);
        lua_rawget(L, LUA_REGISTRYINDEX);
        lua_createtable(L, 0 /* narr */, 4 /* nrec */);
        lua_pushvalue(L, -1);
        ctx->ctx_ref = luaL_ref(L, -3);
        lua_remove(L, -2);
        http_lua_ngx_ctx_add_cleanup(r, ctx->ctx_ref);
//printk("oldtop:%d,now:%d,type:%d\n",top,lua_gettop(L),lua_type(L,-1));

        return 1;
    }

    lua_pushliteral(L, http_lua_ctx_tables_key);
    lua_rawget(L, LUA_REGISTRYINDEX);
    lua_rawgeti(L, -1, ctx->ctx_ref);

    return 1;
}

int
http_lua_ngx_set_ctx(lua_State *L)
{
    http_request_t          *r;
    http_lua_ctx_t          *ctx;

    r = http_lua_get_req(L);
    if (r == NULL) {
        return luaL_error(L, "no request found");
    }

    ctx = http_get_module_ctx(r, HTTP_LUA_MODULE);
    if (ctx == NULL) {
        return luaL_error(L, "no request ctx found");
    }

    return http_lua_ngx_set_ctx_helper(L, r, ctx, 3);
}
int
http_lua_ngx_set_ctx_helper(lua_State *L, http_request_t *r,
    http_lua_ctx_t *ctx, int index)
{
    if (index < 0) {
        index = lua_gettop(L) + index + 1;
    }

    if (ctx->ctx_ref == LUA_NOREF) {

        lua_pushliteral(L, http_lua_ctx_tables_key);
        lua_rawget(L, LUA_REGISTRYINDEX);
        lua_pushvalue(L, index);
        ctx->ctx_ref = luaL_ref(L, -2);
        lua_pop(L, 1);


        return 0;
    }


    lua_pushliteral(L, http_lua_ctx_tables_key);
    lua_rawget(L, LUA_REGISTRYINDEX);
    luaL_unref(L, -1, ctx->ctx_ref);
    lua_pushvalue(L, index);
    ctx->ctx_ref = luaL_ref(L, -2);
    lua_pop(L, 1);

    return 0;
}
static int
http_lua_ngx_ctx_add_cleanup(http_request_t *r, int ref)
{
    lua_State                   *L;
#if CLEANUP_POOL
    httpd_pool_cleanup_t *pclean = httpd_pool_cleanup_add(r->pool,sizeof(ngx_lua_ctx_cleanup_t));
#else
    ngx_lua_ctx_cleanup_t *pclean = (ngx_lua_ctx_cleanup_t *)yaos_malloc(sizeof(ngx_lua_ctx_cleanup_t));
#endif
    if (!pclean) return ENOMEM;

    http_lua_ctx_t          *ctx;
    ctx = http_get_module_ctx(r, HTTP_LUA_MODULE);

    L = http_lua_get_lua_vm(r, ctx); 
#if CLEANUP_POOL
    pclean->handler = ngx_lua_ctx_cleanup;
    ngx_lua_ctx_cleanup_t * data = pclean->data;
    data->L = L;
    data->ref = ref;
#else
    pclean->clean.data = pclean;
    pclean->clean.handler = ngx_lua_ctx_cleanup;
    pclean->L = L;
    pclean->ref = ref;
    http_add_cleanup(r, &pclean->clean);
#endif

    return OK;
}
