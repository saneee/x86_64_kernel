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
    printk("ctx->ctx_ref:%x,%x %s\n", ctx->ctx_ref, LUA_NOREF,http_lua_ctx_tables_key);
    if (ctx->ctx_ref == LUA_NOREF) {

        lua_pushliteral(L, http_lua_ctx_tables_key);
        lua_rawget(L, LUA_REGISTRYINDEX);
        lua_createtable(L, 0 /* narr */, 4 /* nrec */);
        lua_pushvalue(L, -1);
        ctx->ctx_ref = luaL_ref(L, -3);


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

