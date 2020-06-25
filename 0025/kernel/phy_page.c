#include <yaos/types.h>
#include <yaos/kheap.h>
#include <yaos/assert.h>
#include <yaos/spinlock.h>
#include <yaos/printk.h>
#include <asm/phymem.h>
#include <yaos/compiler.h>
#include <asm/pgtable.h>
#define MAX_PHY_PAGE_HOLE   20	
#define MAX_SMALL_HOLE  100
#if 0
#define DEBUG_PRINT printk
#else
#define DEBUG_PRINT inline_printk
#endif
struct phy_page_t;
struct phy_page_t {
    ulong addr;
    ulong size;
    ulong origin_size;
    __thread struct phy_page_t *pnext;
};
__thread static struct phy_page_t *pfreesmall;
__thread static struct phy_page_t *pheadsmall;
static struct phy_page_t phy_page_holes[MAX_PHY_PAGE_HOLE];
__thread static spinlock_t spin_page;
void init_phy_page()
{
    for (int i = 0; i < MAX_PHY_PAGE_HOLE - 1; i++) {
        phy_page_holes[i].pnext = &phy_page_holes[i + 1];
    }
    phy_page_holes[MAX_PHY_PAGE_HOLE - 1].pnext = 0;
    pfreesmall = phy_page_holes;
    pheadsmall = 0;
    init_spinlock(&spin_page);


}

void free_phy_page_small(ulong addr, ulong size)
{
    __thread struct phy_page_t *p;
    __thread struct phy_page_t *ph;

    ASSERT((addr & 0xfff) == 0 && (size & 0xfff) == 0);	//small aligned

    p = pfreesmall;
    if (!p) {
        panic("MAX_PHY_PAGE_HOLE too small!\n");
    }
    spin_lock(&spin_page);
    ph = pheadsmall;
    while(ph) {

        if (ph->addr+ph->size == addr) {
            ph->size += size;
            ph->origin_size += size;
            return;
        }
        ph = ph->pnext;

    };

    pfreesmall = pfreesmall->pnext;
    p->addr = addr;
    p->origin_size = size;
    p->size = size;
    //sort by addr asc,early init can only use low phy_page
    if (!pheadsmall || pheadsmall->addr > p->addr) {
        p->pnext = pheadsmall;
        pheadsmall = p;
        spin_unlock(&spin_page);
        return;
    }

    ph = pheadsmall;
    while (ph->pnext && ph->pnext->addr < p->addr) {
        ph = ph->pnext;
    }
    p->pnext = ph->pnext;
    ph->pnext = p;
    spin_unlock(&spin_page);
    DEBUG_PRINT("PhyPage: Free:%lx,addr,size:%lx\n", addr, size);
}

ulong alloc_phy_page_small(ulong size)
{
    __thread struct phy_page_t *p;
    void *pret;

    //DEBUG_PRINT("alloc page small:%x\n", size);
    if (size & 0xfff)
        size = (size & ~0xfff) + 0x1000;	//align small
    spin_lock(&spin_page);
    p = pheadsmall;
    while (p && p->size < size)
        p = p->pnext;
    while (!p) {
        ulong newaddr;

        spin_unlock(&spin_page);
        newaddr = alloc_phy_page();
        if (!newaddr)
            return 0UL;
        free_phy_page_small(newaddr, PAGE_SIZE);
        spin_lock(&spin_page);
        p = pheadsmall;
        while (p && p->pnext && p->size < size)
            p = p->pnext;

    }
    p->size -= size;
    if (p->size == 0) {
        //add to free link?
        __thread struct phy_page_t *ptr = pheadsmall;

        if (ptr == p) {
            pheadsmall = pheadsmall->pnext;
        }
        else if (ptr->pnext == p) {
            ptr->pnext = p->pnext;
        }
        else {
            while (ptr->pnext && ptr->pnext != p)
                ptr = ptr->pnext;

            if (!ptr->pnext)
                panic("corrupted link,FILE:%s,LINE:%d", __FILE__, __LINE__);
            ptr->pnext = p->pnext;
        }
        p->pnext = pfreesmall;
        pfreesmall = p;
    }
    pret = (void *)p->addr;
    p->addr += size;
    spin_unlock(&spin_page);
    return (ulong)pret;
}


void phy_page_debug()
{
    __thread struct phy_page_t *ptr = pheadsmall;

    printk("phy_page small:\n");
    while (ptr) {
        printk("addr:%lx,size:%lx\n", ptr->addr, ptr->size);
        ptr = ptr->pnext;
    }
}
