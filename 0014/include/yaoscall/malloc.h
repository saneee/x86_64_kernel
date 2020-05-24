#ifndef _YAOSCALL_MALLOC_H
#define _YAOSCALL_MALLOC_H
#include <yaos/yaoscall.h>
#include <yaos/rett.h>


static inline void * yaos_malloc(size_t size)
{
    ret_t (*pfunc)(size_t size)=yaoscall_p(YAOS_malloc);
    ret_t ret=(*pfunc)(size);

    return (void *)ret.v;
}
static inline void  yaos_mfree(void * addr)
{
    void (*pfunc)(void *addr)=yaoscall_p(YAOS_mfree);
    (*pfunc)(addr);
}
#endif
