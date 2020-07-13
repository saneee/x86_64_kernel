#ifndef _MODULE_LUA_HTTP_UTIL_H
#define _MODULE_LUA_HTTP_UTIL_H
#include "common.h"
#define http_lua_ctx_tables_key "_ctx_arr"
extern char lightu_coroutines_key;
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

static inline lua_State *
http_lua_get_lua_vm(http_request_t *r, http_lua_ctx_t *ctx)
{

    if (ctx == NULL) {
        ctx = http_get_module_ctx(r, HTTP_LUA_MODULE);
    }

    if (ctx && ctx->L) {
        return ctx->L;
    }

    return getL();
}
int http_lua_send_chain_link(http_request_t *r, http_lua_ctx_t *ctx, httpd_chain_t *in);
void http_lua_delete_thread(lua_State *L, lua_State *co, int ref);

#endif
