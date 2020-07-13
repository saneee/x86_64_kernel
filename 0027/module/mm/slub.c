#include <yaos/types.h>
#include <yaos/percpu.h>
#include <yaos/compiler.h>
#include <yaos/module.h>
#include <yaos/printk.h>
#include <yaos/init.h>
#include <asm/bitops.h>
#include <yaoscall/page.h>
#include <yaos/yaoscall.h>
#include <yaos/kernel.h>
#include <yaos/assert.h>
#include <yaos/cpupm.h>
#include <asm/pgtable.h>
#include "slub.h"
#define PAGE_4K_MASK 0xfff
#define PAGE_4K_SIZE 0x1000
#define LARGE_IDX 15
#define LARGE_NEXT 14
#define ZERO_SIZE_PTR ((void *)16)
#if 0
#define DEBUG_PRINT printk
#define DEBUG_CALLSTACK() show_call_stack(4)
#else
#define DEBUG_PRINT inline_printk
#define DEBUG_CALLSTACK() 
#endif
#define SLUB_CHECK	0

#if SLUB_CHECK
#define MAX_RECORD 100000
struct record_slub;

struct record_slub{
    struct record_slub *pnext;
    uintptr_t addr;
    size_t size;
};
static struct record_slub records[MAX_RECORD];
static struct record_slub *pfree_slub = NULL;
static struct record_slub *phead_slub=NULL;
static void init_record()
{
    struct record_slub *p = &records[0];
    struct record_slub *pend = &records[MAX_RECORD];
    while (p < pend) {
        p->pnext = (p+1);
        p = p +1;
    }
    p->pnext = 0;
   pfree_slub = &records[0];
}
static void add_record(void *addr,size_t size)
{
    struct record_slub *p = pfree_slub;
    ASSERT(p);
    pfree_slub = p->pnext;
    p->pnext = phead_slub;
    p->addr = (uintptr_t)addr;
    phead_slub = p;
    p->size = size;
}
static void del_record(void *addr)
{
    struct record_slub *p = phead_slub;
    ASSERT(p);
    if (p->addr == (uintptr_t)addr) {
        phead_slub = p->pnext;
        return;
    }
    while (p->pnext) {
        if (p->pnext->addr == (uintptr_t)addr) {
            p->pnext = p->pnext->pnext;
            return; 
        }
        p = p->pnext;

    }
    printk("del_record not found :%016lx\n", addr);
}
void check_slub()
{
   struct record_slub *p = phead_slub;
   while(p) {
      uintptr_t maskaddr = (uintptr_t) p->addr + p->size;
    
      if ((uintptr_t)p->addr != ~*(uintptr_t *)maskaddr) {
        printk("*******************************************check slub fail:%lx,size:%d\n",p->addr, p->size);
        printk("addr:%lx,mask:%lx,maskaddr:%lx\n",p->addr, *(uintptr_t *)maskaddr, maskaddr);
        dump_mem((char *)((uintptr_t)p->addr - 0x1000),0x2000);
      }
      p=p->pnext;
   }
   printk("check slub ok\n");
}

#else
void check_slub()
{
}
#endif
__used static void set_data_break(ulong addr)
{
    printk("@@@@@@@@@@@@@set data break:%lx,rsp:%lx\n",addr,read_rsp());

    ulong dr7 = native_get_debugreg(7);
    set_debugreg(dr7|0x90001,7);
    set_debugreg(addr,0);
    
}

unsigned char *page_bits = NULL;

DECLARE_MODULE(slub_mm, 0, main);
struct freelink {
    void *pnext;
    uintptr_t mask;
};
bool slub_ok = false;
__percpu_data static struct kmem_cache kmalloc_caches[LARGE_IDX + 1];
__percpu_data static ulong installed = 0xd5d5;
static s8 size_index[24] = {
#if 0
    3,                          /* 8 */
#else
// at least 16 size
    4,
#endif
    4,                          /* 16 */
    5,                          /* 24 */
    5,                          /* 32 */
    6,                          /* 40 */
    6,                          /* 48 */
    6,                          /* 56 */
    6,                          /* 64 */
    1,                          /* 72 */
    1,                          /* 80 */
    1,                          /* 88 */
    1,                          /* 96 */
    7,                          /* 104 */
    7,                          /* 112 */
    7,                          /* 120 */
    7,                          /* 128 */
    2,                          /* 136 */
    2,                          /* 144 */
    2,                          /* 152 */
    2,                          /* 160 */
    2,                          /* 168 */
    2,                          /* 176 */
    2,                          /* 184 */
    2                           /* 192 */
};

static struct {
    const char *name;
    unsigned long size;
} const kmalloc_info[] __initconst = {
    {NULL, 0}, {"kmalloc-96", 96},
    {"kmalloc-192", 192}, {"kmalloc-8", 8},
    {"kmalloc-16", 16}, {"kmalloc-32", 32},
    {"kmalloc-64", 64}, {"kmalloc-128", 128},
    {"kmalloc-256", 256}, {"kmalloc-512", 512},
    {"kmalloc-1024", 1024}, {"kmalloc-2048", 2048},
    {"kmalloc-4096", 4096}, {"kmalloc-8192", 8192},
    {"kmalloc-16384", 16384}, {"kmalloc-32768", 32768},
    {"kmalloc-65536", 65536}, {"kmalloc-131072", 131072},
    {"kmalloc-262144", 262144}, {"kmalloc-524288", 524288},
    {"kmalloc-1048576", 1048576}, {"kmalloc-2097152", 2097152},
    {"kmalloc-4194304", 4194304}, {"kmalloc-8388608", 8388608},
    {"kmalloc-16777216", 16777216}, {"kmalloc-33554432", 33554432},
    {"kmalloc-67108864", 67108864}
};

static inline int size_index_elem(size_t bytes)
{
    return (bytes - 1) / 8;
}

struct kmem_cache *kmalloc_slab(size_t size)
{
    int index;

    if (unlikely(size > KMALLOC_MAX_SIZE)) {
        return NULL;
    }

    if (size <= 192) {
        if (!size)
            return ZERO_SIZE_PTR;

        index = size_index[size_index_elem(size)];
    }
    else
        index = fls(size - 1);
//printk("&kmalloc_caches[index]:%p,%p\n",&kmalloc_caches[index], this_cpu_ptr(&kmalloc_caches[index]));
    return this_cpu_ptr(&kmalloc_caches[index]);
}

static void __init new_kmalloc_cache(int idx)
{
    ASSERT_PRINT(idx<=LARGE_IDX,"idx=%d\n",idx);
    struct kmem_cache *p = this_cpu_ptr(&kmalloc_caches[0]);

    p[idx].name = kmalloc_info[idx].name;
    p[idx].size = kmalloc_info[idx].size;
    p[idx].freehead = NULL;
    p[idx].page = NULL;
    p[idx].idx = (uchar) idx;
    ASSERT(idx < 1 << 16);      //page_bits only 4bit 
    DEBUG_PRINT("Name:%s,size:%d,idx:%d,%lx\n", p[idx].name, p[idx].size, idx,
                p);
}

static void __init create_kmalloc_caches()
{
    int i;

    for (i = KMALLOC_SHIFT_LOW; i <= KMALLOC_SHIFT_HIGH; i++) {
        new_kmalloc_cache(i);

        /*
         * Caches that are not of the two-to-the-power-of size.
         * These have to be created immediately after the
         * earlier power of two caches
         */
        if (KMALLOC_MIN_SIZE <= 32 && i == 6)
            new_kmalloc_cache(1);
        if (KMALLOC_MIN_SIZE <= 64 && i == 7)
            new_kmalloc_cache(2);
    }
    new_kmalloc_cache(LARGE_IDX);
    new_kmalloc_cache(LARGE_NEXT);
}

/* Loop over all objects in a slab */
#define for_each_object(__p, __s, __addr, __objects) \
        for (__p = (__addr); __p < (__addr) + (__objects) * (__s)->size;\
                        __p += (__s)->size)

#define for_each_object_idx(__p, __idx, __s, __addr, __objects) \
        for (__p = (__addr), __idx = 1; __idx <= __objects;\
                        __p += (__s)->size, __idx++)

/* Determine object index from a given position */
static inline int slab_index(void *p, struct kmem_cache *s, void *addr)
{
    return (p - addr) / s->size;
}

static inline size_t slab_ksize(const struct kmem_cache *s)
{
    return s->size;
}
static inline uchar get_idx_from_addr(void *addr)
{
    ulong offset = V2P((ulong)addr) >> PAGE_4K_SHIFT;
    if (offset &1 ) return (page_bits[offset / 2] >> 4) & 0xf;
    return  page_bits[offset / 2] & 0xf;
}
static inline void set_idx_from_addr(void *addr, uchar setidx)
{
    ulong offset = V2P((ulong)addr) >> PAGE_4K_SHIFT;
    uchar idx = page_bits[offset / 2];
    if (offset &1 ) page_bits[offset / 2] = (idx & 0xf) | (setidx<<4);
    else page_bits[offset / 2] = (idx & 0xf0) | setidx;

}
struct kmem_cache *get_cache_from_addr(void *addr)
{
    uchar idx = get_idx_from_addr(addr);
    //DEBUG_PRINT("get_cache_from_addr addr:%lx,idx:%d\n", addr,idx);

    return this_cpu_ptr(&kmalloc_caches[idx]);

}

static void __kmfree(void *p)
{
    if (p == ZERO_SIZE_PTR)
        return;
    ASSERT_PRINT((ulong)p>PHYS_VIRT_START && (ulong)p<PHYS_VIRT_END, "p:%lx\n",p);
    struct kmem_cache *s = get_cache_from_addr(p);
    if (s->idx == LARGE_IDX) {
        size_t size = 0;
        uchar idx = 0;
        ulong offset = V2P((ulong) (p)) >> PAGE_4K_SHIFT;

        do {
            size += PAGE_4K_SIZE;
            offset++;

            if (offset & 1) {
                idx = (page_bits[offset / 2] >> 4) & 0xf;
            }
            else {
                idx = page_bits[offset / 2] & 0xf;
            }

        } while (idx == LARGE_NEXT);
        DEBUG_PRINT("kmfree_large:%lx,size:%lx\n", p, size);
        yaos_heap_free_4k(p, size);
        return;
    }
#if SLUB_CHECK
    
#endif
    DEBUG_PRINT("kmfree:%lx No:%d,s:%lx,%s\n", p, s->idx, s, s->name);
    struct freelink *pfree = (struct freelink *)p;
    ASSERT_PRINT(((ulong)p&(s->size-1))==0,"P=%lx\n",p);

    pfree->pnext = s->freehead;
#if SLUB_CHECK
    uintptr_t *pmask = (uintptr_t *)((uintptr_t)p + s->size - sizeof(uintptr_t *));
    if (*pmask != ~(uintptr_t)p) {
        printk("slub free check fail:%lx,%lx,%lx,~mask:%lx\n",p, *pmask, pfree->mask, ~pfree->mask);
        panic("slub check fail\n");

    }

    pfree->mask = ~(uintptr_t)pfree;
    del_record(p);
#endif
    s->freehead = pfree;
    ASSERT_PRINT(!s->freehead || ((ulong)s->freehead>PHYS_VIRT_START && (ulong)s->freehead<PHYS_VIRT_END),"%s->freehead:%lx\n",s->freehead);

}

static bool slub_add_page(struct kmem_cache *s)
{
    void *addr = yaos_heap_alloc_4k(PAGE_4K_SIZE);

    DEBUG_PRINT("s:%lx No.%d add page:%lx,size:%lx\n", s, s->idx, addr,
                s->size);
    ASSERT_PRINT((ulong)addr>PHYS_VIRT_START && (ulong)addr<PHYS_VIRT_END,"addr:%lx\n",addr);

    if (!addr)
        return false;
    struct freelink *pfree = (struct freelink *)addr;
    struct freelink *pend = (struct freelink *)(addr + PAGE_4K_SIZE - s->size );
    struct freelink *pnext = pfree;
    pnext = (struct freelink *)((ulong) (pfree) + s->size);
#if SLUB_CHECK
    pfree->mask = ~(uintptr_t)pfree;
    //printk("p:%lx,pnext:%lx,mask:%lx\n",pfree,pnext,pfree->mask);
#endif

    while (pnext <= pend) {
        pfree->pnext = pnext;
        pfree = pnext;

#if SLUB_CHECK
        pfree->mask = ~(uintptr_t)pfree;
        //printk("p:%lx,pnext:%lx,mask:%lx\n",pfree,pnext,pfree->mask);

#endif
        pfree = pnext;
        pnext = (struct freelink *)((ulong) (pfree) + s->size);

    }
    pfree->pnext = s->freehead;
    
    s->freehead = (struct freelink *)addr;
    ASSERT_PRINT(!s->freehead || ((ulong)s->freehead>PHYS_VIRT_START && (ulong)s->freehead<PHYS_VIRT_END),"s->freehead:%lx\n",s->freehead);


//printk("s->idx:%d\n",s->idx);
    ASSERT(s->idx < 16);        // only 4 bit
    set_idx_from_addr(addr,s->idx);
    return true;
}

static void *__kmalloc(size_t size)
{
#if SLUB_CHECK
    size += 8;
#endif    
    struct kmem_cache *s = kmalloc_slab(size);

//    DEBUG_PRINT("__kmalloc:0x%lx,%lx:%d,size:%d,cpu_base:%016lx,installed:%x\n", size, s, s->idx,s->size,this_cpu_read(this_cpu_off),this_cpu_read(installed));
    ASSERT(this_cpu_read(installed));
    if (s == ZERO_SIZE_PTR) {
        return ZERO_SIZE_PTR;
    }
    if (!s) {
        goto nomem;
    }
    struct freelink *pfree = s->freehead;
   
    if (!pfree) {
        if (!slub_add_page(s))
            goto nomem;
        pfree = s->freehead;
        ASSERT(pfree);
    }
    ASSERT_PRINT((ulong)pfree>PHYS_VIRT_START && (ulong)pfree<PHYS_VIRT_END, "pfree:%lx\n",pfree);
   
#if SLUB_CHECK
    if (~pfree->mask != (uintptr_t)pfree) {
        printk("slub check fail:%lx,%lx,%lx\n",pfree, pfree->mask, pfree->pnext);
        panic("slub check fail\n");
    }

#endif
    s->freehead = pfree->pnext;
    ASSERT_PRINT(!s->freehead || ((ulong)s->freehead>PHYS_VIRT_START && (ulong)s->freehead<PHYS_VIRT_END),"s->freehead:%lx, pfree:%lx,~mask:%lx\n",s->freehead,pfree,~(uintptr_t)s->freehead);
    if (((uintptr_t)pfree&0xfff)+s->size>0x1000) printk("pfree:%lx,size:%d,%d\n",pfree,size,s->size);
    ASSERT(s->size>=0x1000 || ((uintptr_t)pfree&0xfff)+s->size<=0x1000);
#if SLUB_CHECK
    uintptr_t *pmask = (uintptr_t *)((uintptr_t)pfree + s->size - sizeof(uintptr_t *));
    *pmask = ~(uintptr_t)pfree;
    add_record(pfree,s->size-8);

#endif
    DEBUG_PRINT("__kmalloc:%lx,size:%d,s->size：%d\n",pfree,size-8,s->size);
    DEBUG_CALLSTACK();
    return pfree;
nomem:
    printk("out of memory\n");
    return NULL;
}

static void *__kmalloc_large(size_t size)
{

    size_t newsize = (size + PAGE_4K_MASK) & ~PAGE_4K_MASK;

    DEBUG_PRINT("__kmalloc_large:0x%lx,%lx\n", size, newsize);
    ulong addr = (ulong) yaos_heap_alloc_4k(newsize);

    if (!addr) {
        DEBUG_PRINT("yaos_heap_alloc_4k return error %s,%d\n", __func__,
                    __LINE__);
        return NULL;
    }
    set_idx_from_addr((char *)addr, LARGE_IDX);
    ulong newaddr = addr;
    newaddr += PAGE_4K_SIZE;
    /* set all page */
    while (newaddr < (addr + newsize)) {
        set_idx_from_addr((char *)newaddr,LARGE_NEXT);
        newaddr += PAGE_4K_SIZE;
    }
    DEBUG_PRINT("alloc_large page offset:0x%x\n",
                V2P((ulong) addr) >> PAGE_4K_SHIFT);
    BUILD_BUG_ON(LARGE_IDX > 15);	//4bit only
    return (void *)addr;
}

static ret_t slub_mfree(void *p)
{
    __kmfree(p);
    ret_t ret = { 0, YAOSCALL_OK };
    return ret;
}

static ret_t slub_malloc(size_t size)
{
    void *addr;

    if (size > KMALLOC_MAX_CACHE_SIZE) {
        addr = __kmalloc_large(size);
    }
    else {
        addr = __kmalloc(size);
    }
    ret_t ret;

    ret.v = (ulong) addr;
    ret.e = ret.v == 0 ? ERR_YAOSCALL_NO_MEM : YAOSCALL_OK;
    return ret;
}

__init static void init_slub_bp()
{
    ulong maxaddr = yaos_max_physical_mem();
    size_t size = maxaddr >> PAGE_4K_SHIFT;

    size /= 2;                  //4bit one 4k page
    size++;
    size += PAGE_4K_SIZE - 1;
    size &= ~PAGE_4K_MASK;
    page_bits = yaos_heap_alloc_4k(size);
    memset(page_bits, 0, size);
#if SLUB_CHECK
    init_record();
#endif

}

__init static void init_slub_ap()
{
    create_kmalloc_caches();
    slub_ok = true;
    this_cpu_write(installed,0xa5a5);
}

static int main(module_t m, ulong t, void *arg)
{
    modeventtype_t env = (modeventtype_t) t;

    if (env == MOD_BPLOAD) {
        slub_ok = false;
//set_data_break((ulong)this_cpu_ptr(&installed));

        printk("installed:%x\n",this_cpu_read(installed));
        init_slub_bp();
        init_slub_ap();
        regist_yaoscall(YAOS_malloc, slub_malloc);
        regist_yaoscall(YAOS_mfree, slub_mfree);

        DEBUG_PRINT("slub_mm, %lx,%lx,%lx,MAX_CACHE:%lx,MAX:%lx\n", m, t, arg,
                    KMALLOC_MAX_CACHE_SIZE, KMALLOC_MAX_SIZE);
    }
    else if (env == MOD_APLOAD) {
        DEBUG_PRINT("slub_mm,ap load\n");
        if (!is_bp()) {         //bp already done in bpload
            init_slub_ap();
        }
    }
    return MOD_ERR_OK;          //MOD_ERR_NEXTLOOP;
}
