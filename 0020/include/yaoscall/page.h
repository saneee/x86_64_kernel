#ifndef _SYSCALL_PAGE_H
#define _SYSCALL_PAGE_H
#define SYSPAGE_ALLOC_4K 0
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
#endif
