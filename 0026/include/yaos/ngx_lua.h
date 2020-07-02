#ifndef _YAOS_NGX_LUA_H
#define _YAOS_NGX_LUA_H
struct http_state;
typedef struct http_state ngx_http_request_t;
static inline  ngx_http_request_t *
ngx_http_lua_get_req(lua_State *L)
{
    return lua_getexdata(L);

}
static ngx_inline void
ngx_http_lua_set_req(lua_State *L, ngx_http_request_t *r)
{
    lua_setexdata(L, (void *) r);
}

#endif
