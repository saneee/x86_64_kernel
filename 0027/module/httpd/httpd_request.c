#include <types.h>
#include <yaoscall/malloc.h>
#include <yaos/lua_module.h>
#include <yaos/percpu.h>
#include <yaos/sched.h>
#include "httpd_request.h"
#include "palloc.h"
#include <yaos/init.h>
#define HTTPD_POOL_SIZE	4093UL
static int http_request_cleanup_poll_cb(struct poll_cb *p)
{
    http_request_t *r = p->data;
#if CLEANUP_POOL
    httpd_destroy_pool(r->pool);
#else
    http_cleanup_t *cleanup = r->cleanup;
    http_cleanup_t *old;
    while(cleanup) {
       cleanup->handler(cleanup->data);
       old = cleanup;
       cleanup = cleanup->next;
       yaos_mfree(old);
    }
    httpd_destroy_pool(r->pool);

#endif
    return 0;
}
DEFINE_PER_CPU(struct poll_cb, clean_poll)={.handle = http_request_cleanup_poll_cb,.pnext=(struct poll_cb *)0x1234};
/*
http_request_cleanup maybe called in lua, cleanup next tick
*/
void http_request_cleanup(http_request_t *r)
{
    struct poll_cb *p = this_cpu_ptr(&clean_poll);
    p->data = r;
    ASSERT_PRINT(p->handle == http_request_cleanup_poll_cb, "p=%lx,p->handle:%lx,%lx,next:%lx\n",p,p->handle,http_request_cleanup_poll_cb,p->pnext);
    p->pnext = NULL;
    p->thread = current;
    thread_add_poll(p,POLL_LVL_LUA);
  
}

http_request_t * http_create_request(struct http_state *hs)
{
    httpd_pool_t * pool = httpd_create_pool(HTTPD_POOL_SIZE);
    if (!pool) {
        printk("httpd_create_pool fail\n");
        return NULL;
    }

    http_request_t * p = httpd_pcalloc(pool,sizeof(http_request_t));
    if (!p) {
        httpd_destroy_pool(pool);
        return NULL;
    }
    p->ctx = httpd_pcalloc(pool, sizeof(void *) * MAX_HTTP_MODULE); 
    p->pool = pool;
    p->hs = hs;
    p->cleanup = NULL;
    return p;
}
int httpd_lua_handle(struct http_state *hs, http_request_t *r)
{
    lua_State * L = getL();
    //printk("r:%016lx, L:%016lx\n", r, L);
    int http_lua_content_by_file(const char *name, lua_State *L, http_request_t *r);
    return http_lua_content_by_file("httpd", L, r);
}
int httpd_output_filter(http_request_t *r, httpd_chain_t *in)
{
    int http_send_content(struct http_state *hs, httpd_pool_t *pool,httpd_chain_t *in);
    return http_send_content(r->hs, r->pool,in);


}
static __init int init_request_call(bool isbp)
{
    struct poll_cb *p = this_cpu_ptr(&clean_poll);
    p->handle = http_request_cleanup_poll_cb;
    return 0;
}

core_initcall(init_request_call);
