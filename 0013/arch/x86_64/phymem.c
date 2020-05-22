#include <yaos/types.h>
#include <asm/bitops.h>
#include <yaos/printk.h>
#include <asm/pgtable.h>
#include <yaos/kheap.h>
#include <asm/pm64.h>
#include <yaos/assert.h>
#include <string.h>
#if 1
#define DEBUG_PRINTK printk
#else
#define DEBUG_PRINTK inline_printk
#endif

extern ulong __max_phy_addr;
static ulong *pgbits;
static ulong *pmax;
static ulong *pnow;
static ulong nowbit;
static ulong maxbits;
static ulong pages;

#undef NULL_PTR
#define NULL_PTR ((ulong *)0)
static inline ulong *get_none_zero()
{
    ulong *pos;

    if (*pnow)
        return pnow;
    pos = pnow;
    while (++pnow < pmax) {
        if (*pnow)
            return pnow;
    }
    pnow = pgbits;
    if (*pnow)
        return pnow;
    while (++pnow < pos) {
        if (*pnow)
            return pnow;
    }
    return NULL_PTR;
}

ulong alloc_phy_page()
{
    ulong *p;
    int pos;

    while (pages) {
        p = get_none_zero();
        if (p == NULL_PTR)
            return 0;
        pos = ffs(*p);
        if (pos <= 0)
            continue;
        if (test_and_clear_bit(--pos, p)) {
            pages--;
            return PAGE_SIZE * (pos + (p - pgbits) * 64);
        }

    }

    return 0;
}

void free_phy_one_page(ulong addr)
{
    ulong pos;
    extern ulong __max_phy_mem_addr;
    DEBUG_PRINTK("phy mem00: free one page at %lx,pages:%d\n", addr, pages);
    ASSERT(addr<__max_phy_mem_addr);
    if (addr) {                 /*can't free first page */
        /*page align */

        ASSERT((addr & (PAGE_SIZE - 1)) == 0 && addr < __max_phy_addr);
        pos = addr / PAGE_SIZE;
        if (test_and_set_bit(pos, pgbits)) {
            printk("addr:%lx already released\n", addr);
        }
        else
            pages++;
    }
}

void free_phy_pages(ulong addr, size_t size)
{
    ulong pos;

    DEBUG_PRINTK("phy mem: free  pages at %lx,size:%lx\n", addr, size);

    if (addr) {
        ASSERT((addr & (PAGE_SIZE - 1)) == 0 && addr < __max_phy_addr);
        ASSERT((size & (PAGE_SIZE - 1)) == 0);
        pos = addr / PAGE_SIZE;
        while (size > 0) {
            if (test_and_set_bit(pos, pgbits)) {
                printk("addr:%lx already released\n", addr);
            }
            else {
                pages++;
            }
            pos++;
            size -= PAGE_SIZE;
        }
    }

}

void init_phy_mem()
{
    ulong bits = __max_phy_addr / PAGE_SIZE;
    size_t size = bits / 8 + 8;
    char *p = kalloc(size);

    maxbits = bits;
    memset(p, 0, size);

    pgbits = (ulong *) p;
    pnow = pgbits;
    pmax = pgbits + bits / 64;
    nowbit = 0;
    pages = 0;
}
