#ifndef _SYSCALL_PAGE_H
#define _SYSCALL_PAGE_H
#define SYSPAGE_ALLOC_4K 0
#include <types.h>
#include <yaos/yaoscall.h>
#include <yaos/rett.h>
#include <asm/pgtable.h>
static inline void * yaos_p2v(void *p)
{
    return (void *)((ulong)p+KERNEL_BASE);
}
static inline void * yaos_v2p(void *p)
{
    return (void *)((ulong)p-KERNEL_BASE);
}
static inline void * yaos_heap_alloc_4k(size_t size)
{
    ret_t (*pfunc)(size_t size)=yaoscall_p(YAOS_heap_alloc);
    ret_t ret=(*pfunc)(size);
    return (void *)ret.v;
}
static inline void yaos_heap_free_4k(void *addr,size_t size)
{
    void (*pfunc)(void *addr,size_t size)=yaoscall_p(YAOS_heap_free);
    (*pfunc)(addr,size);

}
static inline void * yaos_page_alloc(size_t size)
{
    ret_t (*pfunc)(size_t size)=yaoscall_p(YAOS_page_alloc);
    ret_t ret=(*pfunc)(size);

    return (void *)ret.v;
}
static inline void * yaos_alloc_linear(size_t size)
{
    ret_t (*pfunc)(size_t size)=yaoscall_p(YAOS_alloc_linear);
    ret_t ret=(*pfunc)(size);

    return (void *)ret.v;
}
static inline ret_t yaos_map_p2v(ulong paddr, ulong addr, size_t size, ulong flag)
{
    ret_t (*pfunc)(ulong paddr, ulong addr,size_t size,ulong flag)=yaoscall_p(YAOS_map_p2v);
    ret_t ret=(*pfunc)(paddr,addr,size,flag);
    return ret;

}
static inline ret_t yaos_alloc_at(ulong addr,size_t size,ulong flag)
{
    ret_t (*pfunc)(ulong addr,size_t size,ulong flag)=yaoscall_p(YAOS_alloc_at);
    ret_t ret=(*pfunc)(addr,size,flag);
    return ret;

}
static inline ret_t yaos_free_at(ulong addr,size_t size)
{
    ret_t (*pfunc)(ulong addr,size_t size)=yaoscall_p(YAOS_free_at);
    ret_t ret=(*pfunc)(addr,size);
    return ret;

}
static inline ulong yaos_map_flags_at(ulong addr)
{
    ulong (*pfunc)(ulong addr)=yaoscall_p(YAOS_map_flags_at);
    return (*pfunc)(addr);
}
static inline ret_t yaos_free_linear(ulong addr,size_t size)
{
    ret_t (*pfunc)(ulong addr,size_t size)=yaoscall_p(YAOS_free_linear);
    ret_t ret=(*pfunc)(addr,size);
    return ret;

}

static inline void  yaos_page_free(unsigned long addr,size_t size)
{
    void (*pfunc)(unsigned long addr,size_t size)=yaoscall_p(YAOS_page_alloc);
    (*pfunc)(addr,size);
}
static inline size_t yaos_max_physical_mem()
{
    ret_t (*pfunc)()=yaoscall_p(YAOS_max_physical_mem);
    ret_t ret=(*pfunc)();
    return ret.v;

}
static inline int yaos_map_copy(ulong vaddr, ulong old, size_t size)
{
    int (*pfunc)(ulong vaddr, ulong old, size_t size) = yaoscall_p(YAOS_map_copy);
    return (*pfunc)(vaddr, old, size);
}
#endif
