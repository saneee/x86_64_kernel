#ifndef _YAOS_KHEAP_H
#define _YAOS_KHEAP_H
#define MAX_SMALL_HEAP 2048
void init_kheap(void);
void free_kheap_4k(ulong addr,ulong size);
void * alloc_kheap_4k_low4g(ulong size);
void * alloc_kheap_4k(ulong size);
void * alloc_kheap_small(ulong size);
void free_kheap_small(ulong addr,ulong size);
static inline void * kalloc(ulong size){
    return (size<MAX_SMALL_HEAP)?alloc_kheap_small(size):alloc_kheap_4k(size);
};
ulong alloc_phy_page_small(ulong size);
void free_phy_page_small(ulong addr, ulong size);
#endif
