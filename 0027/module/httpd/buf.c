#include <types.h>
#include <errno.h>
#include <yaoscall/malloc.h>
#include <yaos/printk.h>
#include "palloc.h"
#include "httpd_request.h"
httpd_chain_t *
httpd_alloc_chain_link(httpd_pool_t *pool)
{
    httpd_chain_t  *cl;

    cl = pool->chain;

    if (cl) {
        pool->chain = cl->next;
        return cl;
    }

    cl = httpd_palloc(pool, sizeof(httpd_chain_t));
    if (cl == NULL) {
        return NULL;
    }

    return cl;
}
httpd_chain_t *
httpd_create_chain_of_bufs(httpd_pool_t *pool, httpd_bufs_t *bufs)
{
    u_char       *p;
    int     i;
    httpd_buf_t    *b;
    httpd_chain_t  *chain, *cl, **ll;

    p = httpd_palloc(pool, bufs->num * bufs->size);
    if (p == NULL) { 
        return NULL;
    }

    ll = &chain;

    for (i = 0; i < bufs->num; i++) {

        b = httpd_pcalloc(pool, sizeof(httpd_buf_t));

        if (b == NULL) {
            return NULL;
        }
        b->pos = p;
        b->last = p;

        b->start = p;
        p += bufs->size;
        b->end = p;

        cl = httpd_alloc_chain_link(pool);
        if (cl == NULL) {
            return NULL;
        }

        cl->buf = b;
        *ll = cl;
        ll = &cl->next;
    }

    *ll = NULL;

    return chain;
}
httpd_chain_t *
httpd_chain_get_free_buf(httpd_pool_t *p, httpd_chain_t **free)
{
    httpd_chain_t  *cl;

    if (*free) {
        cl = *free;
        *free = cl->next;
        cl->next = NULL;
        return cl;
    }

    cl = httpd_alloc_chain_link(p);
    if (cl == NULL) {
        return NULL;
    }

    cl->buf = httpd_pcalloc(p, sizeof(httpd_buf_t));

    if (cl->buf == NULL) {
        return NULL;
    }

    cl->next = NULL;

    return cl;
}
int
httpd_chain_add_copy(httpd_pool_t *pool, httpd_chain_t **chain, httpd_chain_t *in)
{
    httpd_chain_t  *cl, **ll;

    ll = chain;

    for (cl = *chain; cl; cl = cl->next) {
        ll = &cl->next;
    }

    while (in) {
        cl = httpd_alloc_chain_link(pool);
        if (cl == NULL) {
            *ll = NULL;
            return ENOMEM;
        }

        cl->buf = in->buf;
        *ll = cl;
        ll = &cl->next;
        in = in->next;
    }

    *ll = NULL;

    return 0;
}

