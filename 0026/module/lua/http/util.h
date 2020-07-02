#ifndef _MODULE_LUA_HTTP_UTIL_H
#define _MODULE_LUA_HTTP_UTIL_H
#include "common.h"
#define http_lua_ctx_tables_key "_ctx_arr"
#define http_lua_coroutines_key v2lightu(&lightu_coroutines_key)
void http_lua_init_vm(lua_State *L);
lua_State * http_lua_new_thread(lua_State *L, int *ref);

struct http_state;
extern http_lua_ctx_t * http_lua_create_ctx(http_request_t *r);
void http_lua_reset_ctx(http_request_t *r, lua_State *L, http_lua_ctx_t *ctx);
static inline void
http_lua_get_globals_table(lua_State *L)
{
    lua_pushvalue(L, LUA_GLOBALSINDEX);
}
static inline void
http_lua_set_req(lua_State *L, http_request_t *r)
{
    lua_setexdata(L, (void *) r);
}
static inline http_request_t *
http_lua_get_req(lua_State *L)
{
    return lua_getexdata(L);
}
int
http_lua_run_thread(lua_State *L, http_request_t *r,
    http_lua_ctx_t *ctx, volatile int nrets);
#endif
