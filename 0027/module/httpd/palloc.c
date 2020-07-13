
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */

#include <types.h>
#include <yaos/printk.h>
#include <yaoscall/malloc.h>
#include <string.h>
#include <asm/pgtable.h>
#include <api/errno.h>
#include "palloc.h"
#if 0
#define DEBUG_PRINT printk
#define DEBUG_CALLSTACK() show_call_stack(4)
#else
#define DEBUG_PRINT inline_printk
#define DEBUG_CALLSTACK()
#endif

static inline void *httpd_palloc_small(httpd_pool_t *pool, size_t size,
    uint align);
static void *httpd_palloc_block(httpd_pool_t *pool, size_t size);
static void *httpd_palloc_large(httpd_pool_t *pool, size_t size);


httpd_pool_t *
httpd_create_pool(size_t size)
{
    httpd_pool_t  *p;

    p = httpd_memalign(HTTPD_POOL_ALIGNMENT, size);
    if (p == NULL) {
        return NULL;
    }

    p->d.last = (u_char *) p + sizeof(httpd_pool_t);
    p->d.end = (u_char *) p + size;
    p->d.next = NULL;
    p->d.failed = 0;

    size = size - sizeof(httpd_pool_t);
    p->max = (size < HTTPD_MAX_ALLOC_FROM_POOL) ? size : HTTPD_MAX_ALLOC_FROM_POOL;

    p->now = p;
    p->chain = NULL;
    p->large = NULL;
    p->cleanup = NULL;


    return p;
}


void
httpd_destroy_pool(httpd_pool_t *pool)
{
    httpd_pool_t          *p, *n;
    httpd_pool_large_t    *l;
    httpd_pool_cleanup_t  *c;

    for (c = pool->cleanup; c; c = c->next) {
        if (c->handler) {
            DEBUG_PRINT("httpd_destroy_pool run cleanup: %p", c);
            c->handler(c->data);
        }
    }

#if (HTTPD_DEBUG)

    /*
     * we could allocate the pool->log from this pool
     * so we cannot use this log while free()ing the pool
     */

    for (l = pool->large; l; l = l->next) {
        DEBUG_PRINT("httpd_destroy_pool free: %p", l->alloc);
    }

    for (p = pool, n = pool->d.next; /* void */; p = n, n = n->d.next) {
        DEBUG_PRINT("httpd_destroy_pool free: %p, unused: %uz", p, p->d.end - p->d.last);

        if (n == NULL) {
            break;
        }
    }

#endif

    for (l = pool->large; l; l = l->next) {
        if (l->alloc) {
            httpd_free(l->alloc);
        }
    }

    for (p = pool, n = pool->d.next; /* void */; p = n, n = n->d.next) {
        httpd_free(p);

        if (n == NULL) {
            break;
        }
    }
}


void
httpd_reset_pool(httpd_pool_t *pool)
{
    httpd_pool_t        *p;
    httpd_pool_large_t  *l;

    for (l = pool->large; l; l = l->next) {
        if (l->alloc) {
            httpd_free(l->alloc);
        }
    }

    for (p = pool; p; p = p->d.next) {
        p->d.last = (u_char *) p + sizeof(httpd_pool_t);
        p->d.failed = 0;
    }

    pool->now = pool;
    pool->chain = NULL;
    pool->large = NULL;
}


void *
httpd_palloc(httpd_pool_t *pool, size_t size)
{
#if !(HTTPD_DEBUG_PALLOC)
    if (size <= pool->max) {
        return httpd_palloc_small(pool, size, 1);
    }
#endif

    return httpd_palloc_large(pool, size);
}


void *
httpd_pnalloc(httpd_pool_t *pool, size_t size)
{
#if !(HTTPD_DEBUG_PALLOC)
    if (size <= pool->max) {
        return httpd_palloc_small(pool, size, 0);
    }
#endif

    return httpd_palloc_large(pool, size);
}


static inline void *
httpd_palloc_small(httpd_pool_t *pool, size_t size, uint align)
{
    u_char      *m;
    httpd_pool_t  *p;

    p = pool->now;

    do {
        m = p->d.last;

        if (align) {
            m = httpd_align_ptr(m, HTTPD_ALIGNMENT);
        }

        if ((size_t) (p->d.end - m) >= size) {
            p->d.last = m + size;

            return m;
        }

        p = p->d.next;

    } while (p);

    return httpd_palloc_block(pool, size);
}


static void *
httpd_palloc_block(httpd_pool_t *pool, size_t size)
{
    u_char      *m;
    size_t       psize;
    httpd_pool_t  *p, *new;

    psize = (size_t) (pool->d.end - (u_char *) pool);

    m = httpd_memalign(HTTPD_POOL_ALIGNMENT, psize);
    if (m == NULL) {
        return NULL;
    }

    new = (httpd_pool_t *) m;

    new->d.end = m + psize;
    new->d.next = NULL;
    new->d.failed = 0;

    m += sizeof(httpd_pool_data_t);
    m = httpd_align_ptr(m, HTTPD_ALIGNMENT);
    new->d.last = m + size;

    for (p = pool->now; p->d.next; p = p->d.next) {
        if (p->d.failed++ > 4) {
            pool->now = p->d.next;
        }
    }

    p->d.next = new;

    return m;
}


static void *
httpd_palloc_large(httpd_pool_t *pool, size_t size)
{
    void              *p;
    uint         n;
    httpd_pool_large_t  *large;

    p = httpd_alloc(size);
    if (p == NULL) {
        return NULL;
    }

    n = 0;

    for (large = pool->large; large; large = large->next) {
        if (large->alloc == NULL) {
            large->alloc = p;
            return p;
        }

        if (n++ > 3) {
            break;
        }
    }

    large = httpd_palloc_small(pool, sizeof(httpd_pool_large_t), 1);
    if (large == NULL) {
        httpd_free(p);
        return NULL;
    }

    large->alloc = p;
    large->next = pool->large;
    pool->large = large;

    return p;
}


void *
httpd_pmemalign(httpd_pool_t *pool, size_t size, size_t alignment)
{
    void              *p;
    httpd_pool_large_t  *large;

    p = httpd_memalign(alignment, size);
    if (p == NULL) {
        return NULL;
    }

    large = httpd_palloc_small(pool, sizeof(httpd_pool_large_t), 1);
    if (large == NULL) {
        httpd_free(p);
        return NULL;
    }

    large->alloc = p;
    large->next = pool->large;
    pool->large = large;

    return p;
}


int
httpd_pfree(httpd_pool_t *pool, void *p)
{
    httpd_pool_large_t  *l;

    for (l = pool->large; l; l = l->next) {
        if (p == l->alloc) {
            DEBUG_PRINT("httpd_pfree:free: %p", l->alloc);
            httpd_free(l->alloc);
            l->alloc = NULL;

            return 0;
        }
    }

    return ESRCH;
}


void *
httpd_pcalloc(httpd_pool_t *pool, size_t size)
{
    void *p;

    p = httpd_palloc(pool, size);
    if (p) {
        httpd_memzero(p, size);
    }

    return p;
}


httpd_pool_cleanup_t *
httpd_pool_cleanup_add(httpd_pool_t *p, size_t size)
{
    httpd_pool_cleanup_t  *c;

    c = httpd_palloc(p, sizeof(httpd_pool_cleanup_t));
    if (c == NULL) {
        return NULL;
    }

    if (size) {
        c->data = httpd_palloc(p, size);
        if (c->data == NULL) {
            return NULL;
        }

    } else {
        c->data = NULL;
    }

    c->handler = NULL;
    c->next = p->cleanup;

    p->cleanup = c;

    DEBUG_PRINT("httpd_pool_cleanup_add: add cleanup: %p", c);

    return c;
}






