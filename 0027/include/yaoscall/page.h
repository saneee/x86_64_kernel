#ifndef _SYSCALL_PAGE_H
#define _SYSCALL_PAGE_H
#define SYSPAGE_ALLOC_4K 0
#include <types.h>
#include <yaos/yaoscall.h>
#include <yaos/rett.h>
#include <asm/pgtable.h>
#include <yaos/vm.h>
#include <yaos/errno.h>
#define MIN_VM_ALLOC_SIZE      PAGE_SIZE_LARGE
static inline void * yaos_p2v(void *p)
{
    return (void *)(P2V(p));
}
static inline void * yaos_v2p(void *p)
{
    ulong addr =(ulong)p;
    if (addr>KERNEL_BASE) return (void *) K2P(p);
    if (addr >= PHYS_VIRT_START && addr <= PHYS_VIRT_END) return (void *) V2P(p);
    ulong (*pfunc)(ulong addr)=yaoscall_p(YAOS_v2p);
    return (void *)(*pfunc)((ulong)p);

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
static inline void * yaos_alloc_linear(vm_zone_t id,size_t size)
{
    ret_t (*pfunc)(vm_zone_t id,void *addr, size_t size)=yaoscall_p(YAOS_alloc_linear);
    ret_t ret=(*pfunc)(id, NULL, size);

    return (void *)ret.v;
}
static inline void * yaos_alloc_linear_at(vm_zone_t id,void *addr, size_t size)
{
    ret_t (*pfunc)(vm_zone_t id,void *addr, size_t size)=yaoscall_p(YAOS_alloc_linear);
    ret_t ret=(*pfunc)(id, addr,size);

    return (void *)ret.v;
}
static inline void * yaos_alloc_linear_hint(void *addr, size_t size)
{
   vm_zone_t id = get_vm_zone_from_addr(addr);
   if (!id) return NULL;
   return yaos_alloc_linear_at(id, addr, size);
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
static inline ret_t yaos_free_linear(vm_zone_t id, ulong addr,size_t size)
{
    ret_t (*pfunc)(vm_zone_t id,ulong addr,size_t size)=yaoscall_p(YAOS_free_linear);
    ret_t ret=(*pfunc)(id,addr,size);
    return ret;

}
static inline ret_t yaos_free_linear_low_mmap(ulong addr,size_t size)
{
    return yaos_free_linear(VM_ZONE_LOW_MMAP,addr,size);
}
static inline void * yaos_alloc_linear_low_mmap(void *addr, size_t size)
{
    return yaos_alloc_linear_at(VM_ZONE_LOW_MMAP,addr,size);
}
static inline ret_t yaos_free_linear_hint(void *addr,size_t size)
{
    vm_zone_t id = get_vm_zone_from_addr(addr);
    ASSERT(id);
    return yaos_free_linear(id,(ulong)addr,size);
}

static inline ret_t yaos_vm_unmap_low_mmap(ulong addr, size_t size)
{
    ret_t (*pfunc)(ulong addr,size_t size)=yaoscall_p(YAOS_unmap_at);
    ret_t ret=(*pfunc)(addr,size);
    if (ret.e !=OK ) return ret;
    return yaos_free_linear_low_mmap(addr, size);

}

static inline ret_t yaos_vm_free_low_mmap(ulong addr,size_t size)
{
    ret_t (*pfunc)(ulong addr,size_t size)=yaoscall_p(YAOS_free_at);
    ret_t ret=(*pfunc)(addr,size);
    if (ret.e !=OK ) return ret;
    return yaos_free_linear_low_mmap(addr, size);
}
static inline ret_t yaos_vm_free_hint(void * addr,size_t size)
{
    vm_zone_t id = get_vm_zone_from_addr(addr);
    ASSERT(id);
    
    ret_t (*pfunc)(ulong addr,size_t size)=yaoscall_p(YAOS_free_at);
    ret_t ret=(*pfunc)((ulong)addr,size);
    if (ret.e !=OK ) return ret;
    return yaos_free_linear(id, (ulong)addr, size);
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
