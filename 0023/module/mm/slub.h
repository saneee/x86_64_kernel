#ifndef _MM_SLUB_H
#define _MM_SLUB_H
#include <yaos/gfp.h>
struct slub_page;
struct slub_page {
    void *s_mem;
    struct slub_page *pfirst;
    struct slub_page *pnext;
    void *freelist;
};

#define KMEM_CACHE(__struct, __flags) kmem_cache_create(#__struct,\
                sizeof(struct __struct), __alignof__(struct __struct),\
                (__flags), NULL)
#define MAX_ORDER 11
#define PAGE_4K_SHIFT  	12
#define KMALLOC_SHIFT_HIGH       PAGE_4K_SHIFT
#define KMALLOC_SHIFT_MAX       (MAX_ORDER + PAGE_4K_SHIFT)
#ifndef KMALLOC_SHIFT_LOW
#define KMALLOC_SHIFT_LOW       3
#endif
#ifndef KMALLOC_MIN_SIZE
#define KMALLOC_MIN_SIZE (1 << KMALLOC_SHIFT_LOW)
#endif
#define SLAB_OBJ_MIN_SIZE (KMALLOC_MIN_SIZE < 16 ? \
                               (KMALLOC_MIN_SIZE) : 16)
/* Maximum allocatable size */
#define KMALLOC_MAX_SIZE        (1UL << KMALLOC_SHIFT_MAX)
/* Maximum size for which we actually use a slab cache */
#define KMALLOC_MAX_CACHE_SIZE  (1UL << KMALLOC_SHIFT_HIGH)
/* Maximum order allocatable via the slab allocagtor */
#define KMALLOC_MAX_ORDER       (KMALLOC_SHIFT_MAX - PAGE_4K_SHIFT)



struct kmem_cache {
    struct slub_page *page;
    uchar  idx;
    uchar  resverd;
    ushort flags;
    int size;                   /* The size of an object including meta data */
    const char *name;          /* Name (only for display!) */
    void *freehead;
};
static inline void *virt_to_obj(struct kmem_cache *s, const void *slab_page,
                                const void *x)
{
    return (void *)x - ((x - slab_page) % s->size);
}

/*
 * Figure out which kmalloc slab an allocation of a certain size
 * belongs to.
 * 0 = zero alloc
 * 1 =  65 .. 96 bytes
 * 2 = 129 .. 192 bytes
 * n = 2^(n-1)+1 .. 2^n
 */
static __always_inline int kmalloc_index(size_t size)
{
    if (!size)
        return 0;

    if (size <= KMALLOC_MIN_SIZE)
        return KMALLOC_SHIFT_LOW;

    if (KMALLOC_MIN_SIZE <= 32 && size > 64 && size <= 96)
        return 1;
    if (KMALLOC_MIN_SIZE <= 64 && size > 128 && size <= 192)
        return 2;
    if (size <= 8)
        return 3;
    if (size <= 16)
        return 4;
    if (size <= 32)
        return 5;
    if (size <= 64)
        return 6;
    if (size <= 128)
        return 7;
    if (size <= 256)
        return 8;
    if (size <= 512)
        return 9;
    if (size <= 1024)
        return 10;
    if (size <= 2 * 1024)
        return 11;
    if (size <= 4 * 1024)
        return 12;
    if (size <= 8 * 1024)
        return 13;
    if (size <= 16 * 1024)
        return 14;
    if (size <= 32 * 1024)
        return 15;
    if (size <= 64 * 1024)
        return 16;
    if (size <= 128 * 1024)
        return 17;
    if (size <= 256 * 1024)
        return 18;
    if (size <= 512 * 1024)
        return 19;
    if (size <= 1024 * 1024)
        return 20;
    if (size <= 2 * 1024 * 1024)
        return 21;
    if (size <= 4 * 1024 * 1024)
        return 22;
    if (size <= 8 * 1024 * 1024)
        return 23;
    if (size <= 16 * 1024 * 1024)
        return 24;
    if (size <= 32 * 1024 * 1024)
        return 25;
    if (size <= 64 * 1024 * 1024)
        return 26;
    BUG();

    /* Will never be reached. Needed because the compiler may complain */
    return -1;
}

/**
 * kmalloc - allocate memory
 * @size: how many bytes of memory are required.
 * @flags: the type of memory to allocate.
 *
 * kmalloc is the normal method of allocating memory
 * for objects smaller than page size in the kernel.
 *
 * The @flags argument may be one of:
 *
 * %GFP_USER - Allocate memory on behalf of user.  May sleep.
 *
 * %GFP_KERNEL - Allocate normal kernel ram.  May sleep.
 *
 * %GFP_ATOMIC - Allocation will not sleep.  May use emergency pools.
 *   For example, use this inside interrupt handlers.
 *
 * %GFP_HIGHUSER - Allocate pages from high memory.
 *
 * %GFP_NOIO - Do not do any I/O at all while trying to get memory.
 *
 * %GFP_NOFS - Do not make any fs calls while trying to get memory. *
 * %GFP_NOWAIT - Allocation will not sleep.
 *
 * %__GFP_THISNODE - Allocate node-local memory only.
 *
 * %GFP_DMA - Allocation suitable for DMA.
 *   Should only be used for kmalloc() caches. Otherwise, use a
 *   slab created with SLAB_DMA.
 *
 * Also it is possible to set different flags by OR'ing
 * in one or more of the following additional @flags:
 *
 * %__GFP_COLD - Request cache-cold pages instead of
 *   trying to return cache-warm pages.
 *
 * %__GFP_HIGH - This allocation has high priority and may use emergency pools.
 *
 * %__GFP_NOFAIL - Indicate that this allocation is in no way allowed to fail
 *   (think twice before using).
 *
 * %__GFP_NORETRY - If memory is not immediately available,
 *   then give up at once.
 *
 *
 * %__GFP_NOWARN - If allocation fails, don't issue any warnings.
 *
 * %__GFP_REPEAT - If allocation fails initially, try once more before failing.
 *
 * There are other flags available as well, but these are not intended
 * for general use, and so are not documented here. For a full list of
 * potential flags, always refer to linux/gfp.h.
 */
/*
static __always_inline void *kmalloc(size_t size, gfp_t flags)
{
    if (__builtin_constant_p(size)) {
        if (size > KMALLOC_MAX_CACHE_SIZE)
            return kmalloc_large(size, flags);
    }
    return __kmalloc(size);
}
*/
#endif
