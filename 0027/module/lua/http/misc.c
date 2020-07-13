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
#if 0
#define DEBUG_PRINT printk
#define DEBUG_CALLSTACK() show_call_stack(4)
#else
#define DEBUG_PRINT inline_printk
#define DEBUG_CALLSTACK()
#endif

static int http_lua_ngx_get(lua_State *L);
static int http_lua_ngx_set(lua_State *L);
void
http_lua_inject_misc_api(lua_State *L)
{
    /* ngx. getter and setter */
    lua_createtable(L, 0, 2); /* metatable for .ngx */
    lua_pushcfunction(L, http_lua_ngx_get);
    lua_setfield(L, -2, "__index");
    lua_pushcfunction(L, http_lua_ngx_set);
    lua_setfield(L, -2, "__newindex");
    lua_setmetatable(L, -2);
}
static int
http_lua_ngx_get(lua_State *L)
{
    http_request_t          *r;
    u_char                      *p;
    size_t                       len;
    http_lua_ctx_t          *ctx;

    r = http_lua_get_req(L);
    if (r == NULL) {
        lua_pushnil(L);
        return 1;
    }

    ctx = http_get_module_ctx(r, HTTP_LUA_MODULE);
    if (ctx == NULL) {
        lua_pushnil(L);
        return 1;
    }

    p = (u_char *) luaL_checklstring(L, -1, &len);

    DEBUG_PRINT("ngx get %s\n", p);

    if (len == sizeof("ctx") - 1
        && strncmp(p, "ctx", sizeof("ctx") - 1) == 0)
    {
        return http_lua_ngx_get_ctx(L);
    }

    lua_pushnil(L);
    return 1;
}

static int
http_lua_ngx_set(lua_State *L)
{
    http_request_t          *r;
    u_char                      *p;
    size_t                       len;

    /* we skip the first argument that is the table */
    p = (u_char *) luaL_checklstring(L, 2, &len);

    if (len == sizeof("ctx") - 1
        && strncmp(p, "ctx", sizeof("ctx") - 1) == 0)
    {
        r = http_lua_get_req(L);
        if (r == NULL) {
            return luaL_error(L, "no request object found");
        }

        return http_lua_ngx_set_ctx(L);
    }

    lua_rawset(L, -3);
    return 0;
}

