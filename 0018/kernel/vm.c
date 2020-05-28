#include <yaos/types.h>
#include <yaos/kheap.h>
#include <yaos/assert.h>
#include <yaos/spinlock.h>
#include <yaos/printk.h>
#include <asm/phymem.h>
#include <yaos/compiler.h>
#include <asm/pgtable.h>
#define PAGE_MASK (PAGE_SIZE-1)
#define MAX_LINEAR_HOLE	10
#define MAX_SMALL_HOLE  100
#if 1
#define DEBUG_PRINT printk
#else
#define DEBUG_PRINT inline_printk
#endif
struct linear_t;
struct linear_t {
    ulong addr;
    ulong size;
    ulong orgin_size;
    struct linear_t *pnext;
};
static struct linear_t *pfreeh;
static struct linear_t *phead;
static struct linear_t linear_holes[MAX_LINEAR_HOLE];
static spinlock_t spin_vm;

void free_linear(ulong addr, ulong size)
{
    struct linear_t *p;
    struct linear_t *ph;

    ASSERT((addr & PAGE_MASK) == 0 && (size & PAGE_MASK) == 0);	//page aligned

    p = pfreeh;
    if (!p) {
        panic("MAX_LINEAR_HOLE too small!\n");
    }
    spin_lock(&spin_vm);
    pfreeh = pfreeh->pnext;
    p->addr = addr;
    p->orgin_size = size;
    p->size = size;
    //sort by addr asc,early init can only use low kheap
    if (!phead || phead->addr > p->addr) {
        p->pnext = phead;
        phead = p;
        spin_unlock(&spin_vm);
        return;
    }

    ph = phead;
    while (ph->pnext && ph->pnext->addr < p->addr) {
        ph = ph->pnext;
    }
    p->pnext = ph->pnext;
    ph->pnext = p;
    spin_unlock(&spin_vm);
    DEBUG_PRINT("Linear: Free:%lx,addr,size:%lx\n", addr, size);
}

void *alloc_linear(ulong size)
{
    struct linear_t *p;
    void *pret;

    DEBUG_PRINT("alloc linear:%x\n", size);
    if (size & PAGE_MASK)
        size = (size & ~PAGE_MASK) + PAGE_SIZE;	//align 4k
    spin_lock(&spin_vm);
    p = phead;
    while (p && p->size < size)
        p = p->pnext;
    if (!p) {
        return (void *)0;
    }
    p->size -= size;
    if (p->size == 0) {
        //add to free link?
        struct linear_t *ptr = phead;

        if (ptr == p) {
            phead = phead->pnext;
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
        p->pnext = pfreeh;
        pfreeh = p;
    }
    pret = (void *)p->addr;
    p->addr += size;
    spin_unlock(&spin_vm);
    return pret;
}

void free_vm_page(ulong addr, ulong size)
{
    ulong *pte;
    ulong phsize = 0;

    while (phsize <= size) {
        pte = (ulong *) get_pte_with_addr(addr);
        if (!pte) {
            panic("free none exist phyical memory vm\n");
        }
        free_phy_one_page((*pte) & ~PAGE_MASK);
        phsize += PAGE_SIZE;
    }
}

void free_vm(ulong addr, ulong size)
{
    free_vm_page(addr, size);
    free_linear(addr, size);
}

void *alloc_vm(ulong size)
{
    ulong phsize = 0;
    ulong addr;

    DEBUG_PRINT("Alloc vm:%lx\n", size);
    void *p = alloc_linear(size);

    if (!p)
        return p;
    addr = (ulong) p;
    while (phsize <= size) {
        ulong pg = alloc_phy_page();

        if (!pg) {
            if (phsize) {
                free_vm_page(addr, phsize);
                free_linear(addr, size);
            }
            return (void *)0;
        }
        map_page_p2v_rw(pg, addr);
        addr += PAGE_SIZE;
        phsize += PAGE_SIZE;
    }
    return p;

}

void free_vm_stack(ulong addr, ulong size)
{
    free_vm_page(addr, size);
    free_linear(addr - PAGE_SIZE, size + PAGE_SIZE);
}

void *alloc_vm_stack(ulong size)
{
    ulong phsize = 0;
    ulong addr;

    DEBUG_PRINT("Alloc vm stack:%lx\n", size);
    void *p = alloc_linear(size + PAGE_SIZE);	//one more page

    if (!p)
        return p;
    addr = (ulong) p + PAGE_SIZE;	//the lowest page is reserved for stack overflow check
    while (phsize <= size) {
        ulong pg = alloc_phy_page();

        if (!pg) {
            if (phsize) {
                free_vm_page(addr, phsize);
                free_linear(addr, size);
            }
            return (void *)0;
        }
        map_page_p2v_rw(pg, addr);
        addr += PAGE_SIZE;
        phsize += PAGE_SIZE;
    }
    return (void *)((ulong) p + PAGE_SIZE);

}

void init_vm(ulong addr, ulong size)
{
    for (int i = 0; i < MAX_LINEAR_HOLE - 1; i++) {
        linear_holes[i].pnext = &linear_holes[i + 1];
    }
    linear_holes[MAX_LINEAR_HOLE - 1].pnext = 0;
    pfreeh = linear_holes;
    phead = 0;
    init_spinlock(&spin_vm);
    free_linear(addr, size);

}

void linear_debug()
{
    struct linear_t *ptr = phead;

    printk("linear:\n");
    while (ptr) {
        printk("addr:%lx,size:%lx\n", ptr->addr, ptr->size);
        ptr = ptr->pnext;
    }
}
