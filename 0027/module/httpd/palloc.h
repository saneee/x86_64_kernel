
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _MODULE_HTTPD_PALLOC_H
#define _MODULE_HTTPD_PALLOC_H


typedef struct httpd_pool_s            httpd_pool_t;
typedef struct httpd_chain_s           httpd_chain_t;

#define httpd_align(d, a)     (((d) + (a - 1)) & ~(a - 1))
#define httpd_align_ptr(p, a)                                                   \
    (u_char *) (((uintptr_t) (p) + ((uintptr_t) a - 1)) & ~((uintptr_t) a - 1))


/*
 * HTTPD_MAX_ALLOC_FROM_POOL should be (httpd_pagesize - 1), i.e. 4095 on x86.
 * On Windows NT it decreases a number of locked pages in a kernel.
 */
#define HTTPD_ALIGNMENT 	sizeof(unsigned long)
#define httpd_memalign(align,size)	yaos_malloc(size)
#define httpd_free(addr)	yaos_mfree((void *)(addr))
#define httpd_alloc(size)	yaos_malloc(size)
#define httpd_memzero(addr,size) memset((void*)(addr),0,size)
#define HTTPD_MAX_ALLOC_FROM_POOL  (PAGE_SIZE_SMALL - 1)

#define HTTPD_DEFAULT_POOL_SIZE    (16 * 1024)

#define HTTPD_POOL_ALIGNMENT       16
#define HTTPD_MIN_POOL_SIZE                                                     \
    httpd_align((sizeof(httpd_pool_t) + 2 * sizeof(httpd_pool_large_t)),            \
              HTTPD_POOL_ALIGNMENT)


typedef void (*httpd_pool_cleanup_pt)(void *data);

typedef struct httpd_pool_cleanup_s  httpd_pool_cleanup_t;

struct httpd_pool_cleanup_s {
    httpd_pool_cleanup_pt   handler;
    void                 *data;
    httpd_pool_cleanup_t   *next;
};


typedef struct httpd_pool_large_s  httpd_pool_large_t;

struct httpd_pool_large_s {
    httpd_pool_large_t     *next;
    void                 *alloc;
};


typedef struct {
    uchar               *last;
    uchar               *end;
    httpd_pool_t           *next;
    uint               failed;
} httpd_pool_data_t;


struct httpd_pool_s {
    httpd_pool_data_t       d;
    size_t                max;
    httpd_pool_t           *now;
    httpd_chain_t          *chain;
    httpd_pool_large_t     *large;
    httpd_pool_cleanup_t   *cleanup;
};





httpd_pool_t *httpd_create_pool(size_t size);
void httpd_destroy_pool(httpd_pool_t *pool);
void httpd_reset_pool(httpd_pool_t *pool);

void *httpd_palloc(httpd_pool_t *pool, size_t size);
void *httpd_pnalloc(httpd_pool_t *pool, size_t size);
void *httpd_pcalloc(httpd_pool_t *pool, size_t size);
void *httpd_pmemalign(httpd_pool_t *pool, size_t size, size_t alignment);
int httpd_pfree(httpd_pool_t *pool, void *p);


httpd_pool_cleanup_t *httpd_pool_cleanup_add(httpd_pool_t *p, size_t size);
int httpd_chain_add_copy(httpd_pool_t *pool, httpd_chain_t **chain, httpd_chain_t *in);
httpd_chain_t *
httpd_chain_get_free_buf(httpd_pool_t *p, httpd_chain_t **free);
#endif /* _MODULE_HTTPD_PALLOC_H*/
