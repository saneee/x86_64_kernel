#ifndef _MOD_HTTPD_HTTP_REQUEST_H
#define _MOD_HTTPD_HTTP_REQUEST_H
#include "palloc.h"
#define CLEANUP_POOL 1
#define LUA_THREAD_POOL 1
enum http_module{
    HTTP_LUA_MODULE=0,
    MAX_HTTP_MODULE
};
typedef void (*http_cleanup_pt)(void *data);
typedef struct http_cleanup_s  http_cleanup_t;
struct http_cleanup_s {
    http_cleanup_pt               handler;
    void                             *data;
    http_cleanup_t               *next;
};
typedef struct {
    int    num;
    size_t       size;
} httpd_bufs_t;

struct httpd_buf_s {
    uchar * pos;
    uchar *last;
    uchar *start;
    uchar *end;
     
};
typedef struct httpd_buf_s httpd_buf_t;
struct httpd_chain_s {
    struct httpd_buf_s    *buf;
    httpd_chain_t  *next;

};
typedef struct http_request_s http_request_t;
struct http_request_s {
    struct http_state *hs;
    httpd_pool_t      *pool;
    http_cleanup_t clean;
    http_cleanup_t *cleanup;
    void ** ctx;
};
static inline void http_add_cleanup(http_request_t *r, http_cleanup_t *cleanup)
{
    cleanup->next = r->cleanup;
    r->cleanup = cleanup;
} 
void http_request_cleanup(http_request_t *r);
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
int httpd_output_filter(http_request_t *r,httpd_chain_t *in);
#endif
