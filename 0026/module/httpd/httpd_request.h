#ifndef _MOD_HTTPD_HTTP_REQUEST_H
#define _MOD_HTTPD_HTTP_REQUEST_H
enum http_module{
    HTTP_LUA_MODULE=0,
    MAX_HTTP_MODULE
};
typedef struct http_request_s http_request_t;
struct http_request_s {
    struct http_state *hs;
    void * ctx[MAX_HTTP_MODULE];
};
static inline void * http_get_module_ctx(http_request_t *r, int module)
{
    return r->ctx[module];
}
static inline void http_set_ctx(http_request_t *r, void *c, int module)
{
    r->ctx[module] = c;
}
http_request_t * http_create_request(struct http_state *hs);
int httpd_lua_handle(struct http_state *hs, http_request_t *r);
#endif
