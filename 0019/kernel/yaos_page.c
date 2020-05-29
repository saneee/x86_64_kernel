#include <yaos/yaoscall.h>
#include <yaos/kheap.h>
#include <asm/pgtable.h>
#include <yaos/rett.h>
#include <yaos/kernel.h>
#include <yaoscall/page.h>
#include <yaos/assert.h>
#include <asm/phymem.h>
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

DECLARE_YAOSCALL(YAOS_heap_alloc, heap_alloc_4k);
DECLARE_YAOSCALL(YAOS_heap_free, heap_free_4k);
DECLARE_YAOSCALL(YAOS_page_alloc, page_alloc);
DECLARE_YAOSCALL(YAOS_page_free, page_free);
DECLARE_YAOSCALL(YAOS_max_physical_mem, max_phymem);
