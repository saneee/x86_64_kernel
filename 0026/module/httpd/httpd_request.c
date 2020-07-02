#include <types.h>
#include <yaoscall/malloc.h>
#include <yaos/lua_module.h>
#include "httpd_request.h"
http_request_t * http_create_request(struct http_state *hs)
{
    http_request_t * p = yaos_malloc(sizeof(http_request_t));
    if (!p) return p;
    p->hs = hs;
    for (int i=0; i< MAX_HTTP_MODULE; i++) p->ctx[i] = NULL;
    return p;
}
int httpd_lua_handle(struct http_state *hs, http_request_t *r)
{
    lua_State * L = getL();
    printk("r:%016lx, L:%016lx\n", r, L);
    int http_lua_content_by_file(const char *name, lua_State *L, http_request_t *r);
    return http_lua_content_by_file("httpd", L, r);
}
