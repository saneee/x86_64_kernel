#include <yaos/types.h>
#include <yaos/kheap.h>
#include <yaos/assert.h>
#include <yaos/spinlock.h>
#include <yaos/printk.h>
#include <asm/phymem.h>
#include <yaos/compiler.h>
#include <asm/pgtable.h>
#include <yaos/vm.h>
#include <yaos/errno.h>
#include <yaos/percpu.h>
#define PAGE_MASK (PAGE_SIZE_LARGE-1)
#define MAX_LINEAR_HOLE	100
#define MAX_SMALL_HOLE  100
#ifndef PAGE_SIZE_SMALL
#define PAGE_SIZE_SMALL 0x1000
#endif
#if 0
#define DEBUG_PRINT printk
#else
#define DEBUG_PRINT inline_printk
#endif
#define MAX_STACKS  100
struct stack_owner;
struct stack_owner{
    struct stack_owner *pnext;
    ulong start;
    u32	  size;
    u32   cpu;
    const char *name;
};
//static DEFINE_PERCPU(struct stack_owner *,pStackHead) = 0;
static struct stack_owner *pStackHead=0;
static struct stack_owner *pStackFree=0;
static struct stack_owner stacks[MAX_STACKS];
struct linear_t;
struct linear_t {
    ulong addr;
    ulong size;
    struct linear_t *pnext;
};
struct linear_zone {
    struct linear_t *pfreeh;
    struct linear_t *phead;
    struct linear_t *alloced;
    ulong start;
    ulong end;
    spinlock_t spin_vm;
    const char *name;
    struct linear_t linear_holes[MAX_LINEAR_HOLE];

};
__used static struct linear_zone low_mmap_zone={.name="low"};
__used static struct linear_zone vmalloc_zone={.name="vmalloc"};
__used static struct linear_zone kernel_stack_zone={.name="kernel_stack"};
__used static struct linear_zone kernel_below1g_zone={.name="kernel_below1g"};
void register_stack(ulong start, ulong end, const char *name,u32 cpu)
{
    struct stack_owner * pstack = pStackFree;
    ASSERT(pstack);
    pStackFree = pstack->pnext;
    pstack->name = name;
    pstack->start = start;
    pstack->size  = end - start;
    pstack->cpu = cpu;
    pstack->pnext = pStackHead;
    pStackHead =  pstack;
}
void dump_stack()
{
    struct stack_owner * pstack =  pStackHead;
    while(pstack){
        DEBUG_PRINT("cpu: %d stack:%s, start:%0lx, end:%0lx\n",pstack->cpu, pstack->name,
                 pstack->start, pstack->start+(ulong)pstack->size);
        pstack = pstack->pnext;
    }

}
void find_stack(ulong addr)
{
    struct stack_owner * pstack =  pStackHead;
    while(pstack){
        if (pstack->start<=addr && pstack->start+(ulong)pstack->size >=addr) {
            DEBUG_PRINT("stack:%s, start:%0lx, end:%0lx, addr:%lx\n",pstack->name, 
                 pstack->start, pstack->start+(ulong)pstack->size, addr);
            return;
        }
        pstack = pstack->pnext;
    }
    DEBUG_PRINT("can't found stack :%lx\n",addr);
}
bool check_stack(ulong addr)
{
    struct stack_owner * pstack =  pStackHead;
    while(pstack){
        if (pstack->start<=addr && pstack->start+(ulong)pstack->size >=addr) {
            return true;
        }
        pstack = pstack->pnext;
    }
    return false;
}
static inline struct linear_zone * get_zone_from_addr(ulong addr)
{
    if (addr>=VMALLOC_START && addr<= VMALLOC_END) return &vmalloc_zone;
    if (addr>=LOW_MMAP_START && addr<=LOW_MMAP_END) return &low_mmap_zone;
    if (addr>=KERNEL_BELOW_1G_START && addr<=KERNEL_BELOW_1G_END) return &kernel_below1g_zone;
    panic("error vmzone:%lx\n",addr);
    return NULL;
}
static inline struct linear_zone * get_zone(vm_zone_t id)
{
    if (id==VM_ZONE_VMALLOC) return &vmalloc_zone;
    if (id==VM_ZONE_LOW_MMAP) return &low_mmap_zone;
    if (id==VM_ZONE_KERNEL_STACK) return &kernel_stack_zone;
    if (id==VM_ZONE_KERNEL_BELOW1G) return &kernel_below1g_zone;
    panic("error vmzone_id:%d\n",id);
    return 0;
}
void free_linear_zone(struct linear_zone *z, ulong addr, ulong size)
{
    struct linear_t *p;
    struct linear_t *ph;

    ASSERT_PRINT((addr & PAGE_MASK) == 0,"addr:%lx",addr); //page aligned
    if (size & PAGE_MASK)
        size = (size & ~PAGE_MASK) + PAGE_SIZE_LARGE;

    p = z->pfreeh;
    if (!p) {
        panic("MAX_LINEAR_HOLE too small!\n");
    }
    spin_lock(&z->spin_vm);
    ph = z->phead;
    while(ph) {//try merge

        if (ph->addr+ph->size == addr) {
            ph->size += size;
            return;
        }
        ph = ph->pnext;

    };

    z->pfreeh = z->pfreeh->pnext;
    p->addr = addr;
    p->size = size;
    //sort by addr asc,early init can only use low kheap
    if (!z->phead || z->phead->addr > p->addr) {
        p->pnext = z->phead;
        z->phead = p;
        spin_unlock(&z->spin_vm);
        return;
    }

    ph = z->phead;
    while (ph->pnext && ph->pnext->addr < p->addr) {
        ph = ph->pnext;
    }
    p->pnext = ph->pnext;
    ph->pnext = p;
    spin_unlock(&z->spin_vm);
    DEBUG_PRINT("Linear %s: Free:%lx,addr,size:%lx\n", z->name, addr, size);
}

void free_linear(vm_zone_t zoneid,ulong addr, ulong size)
{
    free_linear_zone(get_zone(zoneid),addr,size);
}
static void * check_alloced(struct linear_zone *z, void *addr, ulong size)
{
    struct linear_t *p = z->alloced;
    ulong baseaddr = (ulong)addr & ~PAGE_MASK;
    ulong offset = ((ulong)addr&PAGE_MASK) + size;
    spin_lock(&z->spin_vm);

    while(p) {
        if (p->addr == baseaddr) {
            ulong addrmask = (ulong)addr&PAGE_MASK;
            if ((addrmask<= p->size && (p->size+size<PAGE_SIZE_LARGE))) {
                baseaddr += p->size;
                p->size += size;
                spin_unlock(&z->spin_vm);
                DEBUG_PRINT("found alloced:%lx,size:%lx,ret:%lx,%lx,offset:%lx\n",p->addr,p->size, baseaddr, size,offset);

                return (void*)baseaddr;
            }
            if (offset < PAGE_SIZE_LARGE && addrmask >= p->size) {
                p->size = offset;
                spin_unlock(&z->spin_vm);
                return addr;
            }
        }
        p = p->pnext;
    }
    spin_unlock(&z->spin_vm);

    return NULL;
}
void *alloc_linear_zone_impl(struct linear_zone *z,void *realaddr, ulong realsize)
{
    struct linear_t *p,*pfree;
    void *pret;
    void *addr;
    ulong size;
    if (realaddr) {
        addr = check_alloced(z, realaddr, realsize);
        if (addr) return addr;
    }
    pfree = z->pfreeh;
    if (!pfree) {
        panic("MAX_LINEAR_HOLE too small!\n");
    }
    
    DEBUG_PRINT("alloc linear:at:%lx,%lx\n", realaddr, realsize);
    if (realsize & PAGE_MASK) {
        size = (realsize & ~PAGE_MASK) + PAGE_SIZE_LARGE;	
    } else {
        size = realsize;
    }
    if ((ulong)realaddr & PAGE_MASK) {
        addr = (void *)((ulong)realaddr & ~PAGE_MASK);
        size += (ulong)realaddr & PAGE_MASK;
    } else {
        addr = realaddr;
    }    
    spin_lock(&z->spin_vm);
    p = z->phead;
    if (realaddr == NULL) {
        while (p && p->size < size) {
            p = p->pnext;
        }
        if (!p || p->size < size) goto alloc_fail;

    } else {
        while (p && p->size < size
             && !((p->addr <= (ulong)addr && p->addr + p->size >= (ulong)addr+ size))) {
             p = p->pnext;
        }
        if (!p || p->size < size || !(p->addr <= (ulong)addr && p->addr + p->size >= (ulong)addr+ size)) {
            spin_unlock(&z->spin_vm);
            return alloc_linear_zone_impl(z, NULL, realsize);
        }
        if (p->addr != (ulong) addr) {
            ulong oldsize = p->size;
            p->size = (ulong) addr - p->addr;
            z->pfreeh = pfree->pnext;
            pfree->addr = (ulong)addr;
            pfree->size = realsize + ((ulong)realaddr & PAGE_MASK);
            pfree->pnext = z->alloced;
            z->alloced = pfree;
            spin_unlock(&z->spin_vm);
            ulong freeaddr = ((ulong)addr + size + PAGE_MASK) & ~PAGE_MASK;
            DEBUG_PRINT("free_linear_zone:%lx,%lx\n",freeaddr, p->addr + oldsize - freeaddr);
            free_linear_zone(z, freeaddr, p->addr + oldsize - freeaddr);
                
            return (void *)((ulong)addr + ((ulong)realaddr & PAGE_MASK));

        }
        // else p->addr == addr, same with addr==null

    }

    p->size -= size;
    if (p->size == 0) {
        //add to free link?
        struct linear_t *ptr = z->phead;

        if (ptr == p) {
            z->phead = z->phead->pnext;
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
        p->pnext = z->pfreeh;
        z->pfreeh = p;
    }
    pret = (void *)p->addr;
    p->addr += size;
    z->pfreeh = pfree->pnext;
    pfree->addr = (ulong)pret;
    pfree->size = realsize + ((ulong)realaddr & PAGE_MASK);
    pfree->pnext = z->alloced;
    z->alloced = pfree;

    spin_unlock(&z->spin_vm);
    return (void *)((ulong)pret + ((ulong)realaddr & PAGE_MASK));

alloc_fail:
    spin_unlock(&z->spin_vm);
    return (void *)0;

}
void *alloc_linear_hint(void *addr, ulong size)
{
    return alloc_linear_zone_impl(get_zone_from_addr((ulong)addr), addr, size);    
}
void free_linear_hint(void *addr, ulong size)
{
    free_linear_zone(get_zone_from_addr((ulong)addr), (ulong)addr, size);  
}
void *alloc_linear_impl(vm_zone_t id, void *addr, ulong size)
{
    return alloc_linear_zone_impl(get_zone(id),addr,size);
}

void free_vm(vm_zone_t id,ulong addr, ulong size)
{
    unmap_free_at(addr, size);
    free_linear(id,addr, size);
}

void *alloc_vm(vm_zone_t id,ulong size)
{
    ulong addr;

    DEBUG_PRINT("Alloc vm%d:%lx\n",id, size);
    void *p = alloc_linear(id,size);

    if (!p)
        return p;
    addr = (ulong) p;
    if (OK!=map_alloc_at(addr,size,map_flags_rw())) {
        free_linear(id,addr,size);
        return 0;
    }
    return p;

}

void free_vm_stack(ulong addr, ulong size)
{
    unmap_free_at(addr, size);
    free_linear(VM_ZONE_KERNEL_STACK, addr - PAGE_SIZE_LARGE, size + PAGE_SIZE_LARGE);
}

void *alloc_vm_stack(ulong size)
{
    ulong addr;

    DEBUG_PRINT("Alloc vm stack:%lx\n", size);
    void *p = alloc_linear(VM_ZONE_KERNEL_STACK, size + PAGE_SIZE_LARGE);	//one more page

    if (!p)
        return p;
    addr = (ulong) p + PAGE_SIZE_LARGE;	//the lowest page is reserved for stack overflow check
    if (OK!=map_alloc_at(addr+size-PAGE_SIZE_SMALL,PAGE_SIZE_SMALL,map_flags_stack())) {
        free_linear(VM_ZONE_KERNEL_STACK,addr-PAGE_SIZE_LARGE,size+PAGE_SIZE_LARGE);
        return 0;
    }
    return (void *)(addr);

}
void init_vm_zone(struct linear_zone *p,ulong addr,ulong size)
{
    for (int i = 0; i < MAX_LINEAR_HOLE - 1; i++) {
        p->linear_holes[i].pnext = &p->linear_holes[i + 1];
    }
    p->linear_holes[MAX_LINEAR_HOLE - 1].pnext = 0;
    p->pfreeh = p->linear_holes;
    p->phead = 0;
    p->alloced = 0;
    init_spinlock(&p->spin_vm);
    free_linear_zone(p,addr, size);

}
void init_stack_owner()
{
     for (int i = 0; i < MAX_STACKS - 1; i++) {
        stacks[i].pnext = &stacks[i + 1];
    }
    stacks[MAX_STACKS - 1].pnext = 0;
    pStackFree = &stacks[0];

}
void init_vm(ulong addr, ulong size)
{
    init_stack_owner();
    init_vm_zone(get_zone(VM_ZONE_VMALLOC), VMALLOC_START, VMALLOC_SIZE);
    init_vm_zone(get_zone(VM_ZONE_LOW_MMAP),LOW_MMAP_START,LOW_MMAP_SIZE);
    init_vm_zone(get_zone(VM_ZONE_KERNEL_STACK),KERNEL_STACK_ZONE_START,KERNEL_STACK_ZONE_SIZE);
    init_vm_zone(get_zone(VM_ZONE_KERNEL_BELOW1G), KERNEL_BELOW_1G_START, KERNEL_BELOW_1G_SIZE);
}

