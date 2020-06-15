#include <yaos/types.h>
#include <yaos/kheap.h>
#include <yaos/assert.h>
#include <yaos/spinlock.h>
#include <yaos/printk.h>
#include <asm/phymem.h>
#include <yaos/compiler.h>
#include <asm/pgtable.h>
#define MAX_HEAP_HOLE   20	
#define MAX_SMALL_HOLE  100
#if 1
#define DEBUG_PRINT printk
#else
#define DEBUG_PRINT inline_printk
#endif
struct kheap_t;
struct kheap_t {
    ulong addr;
    ulong size;
    ulong origin_size;
    __thread struct kheap_t *pnext;
};
__thread static struct kheap_t *pfree4k;
__thread static struct kheap_t *phead4k;
static struct kheap_t heap_holes[MAX_HEAP_HOLE];
static struct kheap_t heap_small_holes[MAX_SMALL_HOLE];
__thread static spinlock_t spin_4k;
__thread static spinlock_t spin_small;
__thread static struct kheap_t *pfree_small;
__thread static struct kheap_t *phead_small;
void init_kheap()
{
    for (int i = 0; i < MAX_HEAP_HOLE - 1; i++) {
        heap_holes[i].pnext = &heap_holes[i + 1];
    }
    heap_holes[MAX_HEAP_HOLE - 1].pnext = 0;
    pfree4k = heap_holes;
    phead4k = 0;
    init_spinlock(&spin_4k);

    for (int i = 0; i < MAX_SMALL_HOLE - 1; i++) {
        heap_small_holes[i].pnext = &heap_small_holes[i + 1];
    }
    heap_small_holes[MAX_SMALL_HOLE - 1].pnext = 0;
    pfree_small = heap_small_holes;
    phead_small = 0;
    init_spinlock(&spin_small);

}

void free_kheap_4k(ulong addr, ulong size)
{
    __thread struct kheap_t *p;
    __thread struct kheap_t *ph;

    ASSERT((addr & 0xfff) == 0 && (size & 0xfff) == 0);	//4k aligned

    p = pfree4k;
    if (!p) {
        panic("MAX_HEAP_HOLE too small!\n");
    }
    spin_lock(&spin_4k);
    ph = phead4k;
    while(ph) {

        if (ph->addr+ph->size == addr) {
            ph->size += size;
            ph->origin_size += size;
            return;
        }
        ph = ph->pnext;

    };

    pfree4k = pfree4k->pnext;
    p->addr = addr;
    p->origin_size = size;
    p->size = size;
    //sort by addr asc,early init can only use low kheap
    if (!phead4k || phead4k->addr > p->addr) {
        p->pnext = phead4k;
        phead4k = p;
        spin_unlock(&spin_4k);
        return;
    }

    ph = phead4k;
    while (ph->pnext && ph->pnext->addr < p->addr) {
        ph = ph->pnext;
    }
    p->pnext = ph->pnext;
    ph->pnext = p;
    spin_unlock(&spin_4k);
    DEBUG_PRINT("KHeap: Free:%lx,addr,size:%lx\n", addr, size);
}

void *alloc_kheap_4k(ulong size)
{
    __thread struct kheap_t *p;
    void *pret;

    DEBUG_PRINT("alloc 4k:%x\n", size);
    if (size & 0xfff)
        size = (size & ~0xfff) + 0x1000;	//align 4k
    spin_lock(&spin_4k);
    p = phead4k;
    while (p && p->size < size)
        p = p->pnext;
    while (!p) {
        ulong newaddr;

        spin_unlock(&spin_4k);
        newaddr = P2V(alloc_phy_page());
        if (!newaddr)
            return (void *)0;
        free_kheap_4k(newaddr, PAGE_SIZE);
        spin_lock(&spin_4k);
        p = phead4k;
printk("p:%p,newaddr:%lx\n",p,newaddr);
        while (p && p->pnext && p->size < size)
            p = p->pnext;

    }
    p->size -= size;
    if (p->size == 0) {
        //add to free link?
        __thread struct kheap_t *ptr = phead4k;

        if (ptr == p) {
            phead4k = phead4k->pnext;
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
        p->pnext = pfree4k;
        pfree4k = p;
    }
    pret = (void *)p->addr;
    p->addr += size;
    spin_unlock(&spin_4k);
    return pret;
}

void free_kheap_small(ulong addr, ulong size)
{
    __thread struct kheap_t *p;

    ASSERT((addr & 0xfff) == 0 && (size & 0xfff) == 0);	//4k aligned
    spin_lock(&spin_small);

    p = pfree_small;
    if (!p) {
        panic("MAX_HEAP_HOLE too small!\n");
    }
    pfree_small = pfree_small->pnext;
    p->addr = addr;
    p->origin_size = size;
    p->size = size;
    p->pnext = phead_small;
    phead_small = p;
    spin_unlock(&spin_small);
    DEBUG_PRINT("KHeap: Small Free:%lx,addr,size:%lx\n", addr, size);
}

void *alloc_kheap_small(ulong size)
{
    __thread struct kheap_t *p;
    void *pret;

    DEBUG_PRINT("alloc small:%lx\n", size);
    ASSERT(size < 0x1000);      //small 
    spin_lock(&spin_small);
    p = phead_small;
    while (p && p->size < size)
        p = p->pnext;
    while (!p) {
        ulong newaddr;

        spin_unlock(&spin_small);
        newaddr = (ulong) alloc_kheap_4k(0x1000);
        if (!newaddr)
            return (void *)0;
        free_kheap_small(newaddr, 0x1000);
        spin_lock(&spin_small);
        p = phead_small;
        while (p && p->size < size)
            p = p->pnext;

    }
    p->size -= size;
    if (p->size <= 16) {        //drop small than 16
        //add to free link?
        __thread struct kheap_t *ptr = phead_small;

        if (ptr == p) {
            phead_small = phead_small->pnext;
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
        p->pnext = pfree_small;
        pfree_small = p;
    }
    pret = (void *)p->addr;
    p->addr += size;
    spin_unlock(&spin_small);
    return pret;
}

void kheap_debug()
{
    __thread struct kheap_t *ptr = phead4k;

    printk("kheap 4k:\n");
    while (ptr) {
        printk("addr:%lx,size:%lx\n", ptr->addr, ptr->size);
        ptr = ptr->pnext;
    }
}
