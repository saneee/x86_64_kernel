#include <yaos/yaoscall.h>
#include <yaos/kheap.h>
#include <asm/pgtable.h>
#include <yaos/rett.h>
#include <yaos/kernel.h>
#include <yaoscall/page.h>
#include <yaos/assert.h>
#include <yaos/vm.h>
#include <asm/phymem.h>
#include <asm/pgtable.h>
#include <yaos/errno.h>
#include <yaos/printk.h>
extern ulong __max_phy_mem_addr;
static ret_t heap_alloc_4k(size_t size);
static ret_t max_phymem()
{
    ret_t ret = { __max_phy_mem_addr, YAOSCALL_OK };
    return ret;
}

static ret_t heap_alloc_4k(size_t size)
{
    ASSERT((size & 0xfff) == 0);	//only 4k aligined 
    ret_t ret;
    void *addr = alloc_kheap_4k(size);

    ret.e = 0;
    ret.v = (ulong) addr;
    return ret;

}

static ret_t heap_free_4k(void *addr, size_t size)
{
    ASSERT((size & 0xfff) == 0 && ((ulong) addr & 0xfff) == 0);	//only 4k aligined
    ret_t ret = { 0, 0 };
    free_kheap_4k((ulong) addr, size);
    return ret;

}

static ret_t page_alloc(size_t size)
{
    ASSERT(size == PAGE_SIZE);  //one page only now.
    ret_t ret;

    ret.v = alloc_phy_page();
    ret.e = ret.v == 0 ? ERR_YAOSCALL_NO_MEM : YAOSCALL_OK;
    return ret;

}

static ret_t page_free(ulong addr, size_t size)
{
    void free_phy_pages(ulong addr, size_t size);
    free_phy_pages(addr, size);
    ret_t ret = { 0, 0 };
    return ret;
}
static ret_t s_alloc_linear(size_t size)
{
    ret_t ret;
    ret.v = (ulong) alloc_linear(size);
    ret.e = ret.v == 0 ? ERR_YAOSCALL_NO_MEM : YAOSCALL_OK;
    return ret;

}
static ret_t s_free_linear(ulong addr, size_t size)
{
    dump_mem64((char *)0xffffffff80009000,0x100);
    free_linear(addr, size);
    ret_t ret = {0,0};
    return ret;
}
static ret_t s_alloc_at(ulong addr, size_t size,ulong flag)
{
    ret_t ret = {0,0};
    if (!addr){
        addr = (ulong) alloc_linear(size);
        if (!addr) {
            ret.e = ENOMEM;
            return ret;
        }
    }
    ret.e = map_alloc_at(addr,size,flag);
    return ret;
}
static ret_t s_free_at(ulong addr, size_t size)
{
    printk("unmap_free_at：%lx,size:%lx\n",addr,size);
    unmap_free_at(addr, size);
    ret_t ret={0,0};
    return ret;
}
  
static ret_t s_map_p2v(ulong paddr, ulong vaddr,size_t size,ulong flag)
{
    ret_t ret = { 0, 0};
    ret.e = map_p2v(paddr,vaddr,size,flag);
    return ret;
}
static int s_copy_map(ulong vaddr, ulong old, size_t size)
{
    return map_copy_map(vaddr, old, size);
}
DECLARE_YAOSCALL(YAOS_heap_alloc, heap_alloc_4k);
DECLARE_YAOSCALL(YAOS_heap_free, heap_free_4k);
DECLARE_YAOSCALL(YAOS_page_alloc, page_alloc);
DECLARE_YAOSCALL(YAOS_page_free, page_free);
DECLARE_YAOSCALL(YAOS_max_physical_mem, max_phymem);
DECLARE_YAOSCALL(YAOS_map_p2v, s_map_p2v);
DECLARE_YAOSCALL(YAOS_alloc_linear, s_alloc_linear);
DECLARE_YAOSCALL(YAOS_free_linear, s_free_linear);
DECLARE_YAOSCALL(YAOS_alloc_at,s_alloc_at);
DECLARE_YAOSCALL(YAOS_free_at,s_free_at);
DECLARE_YAOSCALL(YAOS_map_copy,s_copy_map);
