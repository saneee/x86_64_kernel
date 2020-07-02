#ifndef _ASM_PH_PAGE_H
#define _ASM_PH_PAGE_H
#define IO_MEM_BASE	0xffffffe800000000
#define KERNEL_BASE	0xffffffff80000000
#define __PHYS_VIRT_START  0xFFFF820000000000
#define __VMALLOC_BASE_L4       0xffffc90000000000UL
#define __VMALLOC_BASE_L5       0xffa0000000000000UL
#define __LOW_MMAP_START	0x0000700000000000UL
#define VMALLOC_SIZE_TB_L4      32UL
#define VMALLOC_SIZE_TB_L5      12800UL

#define __VMEMMAP_BASE_L4       0xffffea0000000000UL
#define __VMEMMAP_BASE_L5       0xffd4000000000000UL
# define VMALLOC_START          __VMALLOC_BASE_L4
# define VMALLOC_SIZE_TB        VMALLOC_SIZE_TB_L4
# define VMEMMAP_START          __VMEMMAP_BASE_L4
#define VMALLOC_END             (VMALLOC_START + (VMALLOC_SIZE_TB << 40) - 1)
#define VMALLOC_SIZE	(VMALLOC_SIZE_TB << 40)
#define LOW_MMAP_START  __LOW_MMAP_START
#define LOW_MMAP_SIZE    (4UL<<40)
#define LOW_MMAP_END  (LOW_MMAP_START+(LOW_MMAP_SIZE-1))
#define KERNEL_STACK_ZONE_SIZE (1UL<<40)
#define KERNEL_STACK_ZONE_START (__PHYS_VIRT_START-KERNEL_STACK_ZONE_SIZE)
#define KERNEL_STACK_ZONE_END (KERNEL_STACK_ZONE_START + (KERNEL_STACK_ZONE_SIZE-1))
#define PHYS_VIRT_START __PHYS_VIRT_START
#define PHYS_VIRT_END   (VMALLOC_START - (1ul<<40) -1)
#define P2K(v) __P2K((ulong)(v))
#define K2P(v) __K2P((ulong)(v))
#define P2V(v) __P2V((ulong)(v))
#define V2P(v) __V2P((ulong)(v))
#define KV2P(v) __KV2P((ulong)(v))
static inline ulong __P2K(ulong addr){
    return addr+KERNEL_BASE; 
};
static inline ulong __K2P(ulong addr){
    return addr-KERNEL_BASE;
}
static inline ulong IO2V(ulong addr){
    return addr+IO_MEM_BASE;
}
static inline ulong __P2V(ulong addr) {
    return addr + PHYS_VIRT_START;
}
#include <yaos/assert.h>
static inline ulong __V2P(ulong addr) {
    ASSERT(addr <= PHYS_VIRT_END && addr >=PHYS_VIRT_START);
    return addr - PHYS_VIRT_START;
}
static inline ulong __KV2P(ulong addr) {
    return addr > KERNEL_BASE ? addr - KERNEL_BASE: __V2P(addr);

}
extern void *ioremap_nocache(ulong addr,ulong size);
extern int map_page_p2v(ulong p,ulong v,ulong uflag);
extern int map_4k_page_p2v(ulong p,ulong v,ulong uflag);

extern int map_p2v(ulong p,ulong v,size_t size,ulong flag);
extern u64 get_pte_with_addr(ulong addr);
extern int map_alloc_at(ulong vaddr, size_t size, ulong flag);
extern ulong map_flags_at(ulong vaddr);
extern int map_copy_map(ulong vaddr,ulong old,size_t size);
extern void unmap_free_at(ulong vaddr, size_t size);
extern ulong get_phy_addr(ulong vaddr);
#include <yaos/rett.h>
extern ret_t map_paddr_flags_at(ulong vaddr);

/*2M hug page */
#define PAGE_SIZE_LARGE 0x200000
#define PAGE_SIZE_SMALL 0x1000
#define PAGE_SIZE_HUGE  0x40000000

// Page table/directory entry flags.
#define PTE_P           0x001   // Present
#define PTE_W           0x002   // Writeable
#define PTE_U           0x004   // User
#define PTE_PWT         0x008   // Write-Through
#define PTE_PCD         0x010   // Cache-Disable
#define PTE_A           0x020   // Accessed
#define PTE_D           0x040   // Dirty
#define PTE_PS          0x080   // Page Size
#define PTE_G           0x100   // Page Global
#define PTE_MBZ         0x180   // Bits must be zero

static inline int map_page_p2v_rw(ulong paddr, ulong vaddr)
{
    return map_page_p2v(paddr,vaddr,PTE_P|PTE_W|PTE_PS);
}
static inline ulong map_flags_base()
{
    return PTE_P|PTE_PS;
}
static inline ulong map_flags_rw()
{
    return PTE_P|PTE_W|PTE_PS;
}
static inline ulong map_flags_exec()
{
    return PTE_P|PTE_PS;
}
static inline ulong map_flags_write()
{
    return PTE_W;
}
static inline ulong map_flags_code()
{
    return 0;
}
static inline ulong map_flags_stack()
{
    return map_flags_rw();
}
/*
 * PGDIR_SHIFT determines what a top-level page table entry can map
 */
#define PML4_NR		512
#define PML4_SHIFT     39
#define PML4_PER_PDP    512

/*
 * 3rd level page
 */
#define PDP_SHIFT       30
#define PDP_PER_PD    512

/*
 * PMD_SHIFT determines the size of the area a middle-level
 * page table can map
 */
#define PD_SHIFT       21

#define PD_PER_PTE	512
/*
 * entries per page directory level
 */
#define PTE_SHIFT 	12
#define PTRS_PER_PTE    512
#endif
