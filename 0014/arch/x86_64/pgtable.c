#include <yaos/types.h>
#include <asm/bitops.h>
#include <yaos/printk.h>
#include <asm/pgtable.h>
#include <yaos/errno.h>
#include <asm/cpu.h>
#include <yaos/kheap.h>
#include <asm/pm64.h>
#include <string.h>
#if 1
#define DEBUG_PRINT printk
#else
#define DEBUG_PRINT inline_printk
#endif
#define NUMBER_4G 0x100000000
struct pml4_t {
    u64 pml4e[PML4_PER_PDP];
};
struct pdp_t {
    u64 pdpte[PDP_PER_PD];
};
struct pd_t {

    u64 pde[PD_PER_PTE];
};
ulong bp_cr3;
static struct pml4_t pml4 __attribute__ ((aligned(4096)));;
static struct pdp_t first_pdp __attribute__ ((aligned(4096)));
static struct pdp_t second_pdp __attribute__ ((aligned(4096)));
static struct pd_t first_pd __attribute__ ((aligned(4096)));
static struct pd_t second_pd __attribute__ ((aligned(4096)));
static struct pd_t third_pd __attribute__ ((aligned(4096)));
static struct pd_t fourth_pd __attribute__ ((aligned(4096)));

static void init_pml4()
{
    extern ulong __max_phy_mem_addr;
    int map_page_p2v(ulong paddr, ulong vaddr, ulong flag);
    int idx;
    memset(&pml4, 0, sizeof(pml4));
    memset(&first_pdp, 0, sizeof(first_pdp));
    memset(&second_pdp, 0, sizeof(first_pdp));

    ASSERT((((ulong) & pml4) & 0xfff) == 0);
    ASSERT((((ulong) & first_pdp) & 0xfff) == 0);
    ASSERT((((ulong) & first_pd) & 0xfff) == 0);

    //init first 4G 4*512*2M
    //pml4.pml4e[0] = (V2P((ulong) & first_pdp)) | PTE_P | PTE_W; 
    //first_pdp.pdpte[0] = V2P((ulong) & first_pd) | PTE_P | PTE_W;
    //first_pdp.pdpte[1] = V2P((ulong) & second_pd) | PTE_P | PTE_W;
    //first_pdp.pdpte[2] = V2P((ulong) & third_pd) | PTE_P | PTE_W;
    //first_pdp.pdpte[3] = V2P((ulong) & fourth_pd) | PTE_P | PTE_W;
    //init KERNEL_BASE FIRST 1G or 2G
    pml4.pml4e[(KERNEL_BASE >> PML4_SHIFT) & (PML4_NR - 1)]=V2P((ulong) & second_pdp) | PTE_P | PTE_W;
    idx = (KERNEL_BASE >> PDP_SHIFT) & (PML4_PER_PDP - 1);
    second_pdp.pdpte[idx] = V2P((ulong) & first_pd) | PTE_P | PTE_W; 
    if (idx<PML4_PER_PDP-1) {
        second_pdp.pdpte[idx+1] = V2P((ulong) & second_pd) | PTE_P | PTE_W;
    } 
    for (int i = 0; i < PD_PER_PTE; i++) {
        first_pd.pde[i] = i * PAGE_SIZE | PTE_PS | PTE_W | PTE_P;
        second_pd.pde[i] =
            (i + PD_PER_PTE) * PAGE_SIZE | PTE_PS | PTE_W | PTE_P;
        third_pd.pde[i] =
            (i + 2 * PD_PER_PTE) * PAGE_SIZE | PTE_PS | PTE_W | PTE_P;
        fourth_pd.pde[i] =
            (i + 3 * PD_PER_PTE) * PAGE_SIZE | PTE_PS | PTE_W | PTE_P;
    }
    DEBUG_PRINT("__max_phy_mem_addr:%lx\n", __max_phy_mem_addr);
    /*
    if (__max_phy_mem_addr > PAGE_SIZE) {
        ulong addr = PAGE_SIZE;

        for (; addr + PAGE_SIZE < __max_phy_mem_addr; addr += PAGE_SIZE) {
            if (OK != map_page_p2v(addr, P2V(addr), PTE_PS | PTE_P | PTE_W)) {
                panic("Not enough init heap memory!");
            }
        }
    }
    */
u64 get_phy_addr(u64 vaddr);
//dump_mem(&pml4,0x1000);
printk("write_cr3:%lx,%lx,%lx\n",V2P((ulong) & pml4),&pml4,get_phy_addr((u64)&pml4));
    write_cr3(V2P((ulong) & pml4));
printk(" read cr3\n");
    bp_cr3 = read_cr3();
printk(" new cr3:%x\n",bp_cr3);
}

u64 get_pte_with_addr(u64 addr)
{
    ulong i, j, k, pml4base;
    struct pml4_t *p_pml4;
    struct pdp_t *p_pdp;
    struct pd_t *p_pd;

    pml4base = V2P((ulong)&pml4);//read_cr3();
    pml4base &= ~0xfff;
    
    printf("pml4base %lx ", pml4base);
    i = (addr >> PML4_SHIFT) & (PML4_NR - 1);
    j = (addr >> PDP_SHIFT) & (PML4_PER_PDP - 1);
    k = (addr >> PD_SHIFT) & (PD_PER_PTE - 1);
    p_pml4 = (struct pml4_t *)P2V(pml4base);
    p_pdp = (struct pdp_t *)(p_pml4->pml4e[i] & ~0xfff);
    printf(" %lx ", p_pdp);
    if (!p_pdp)
        return 0;
    else p_pdp = (struct pdp_t *)P2V(p_pdp);
    p_pd = (struct pd_t *)(p_pdp->pdpte[j] & ~0xfff);
    printf(" %lx ", p_pd);
    printf("%d %d %d \n", i, j, k);
    if (!p_pd)
        return 0;
    else p_pd = (struct pd_t *)P2V(p_pd);

    return (u64) & p_pd->pde[k];
}
u64 get_phy_addr(u64 vaddr)
{
    u64 * p=(u64 *)get_pte_with_addr(vaddr);
    if (p)return *p;
    return 0;
}
int map_page_p2v(ulong paddr, ulong vaddr, ulong flag)
{
    ulong i, j, k;
    struct pdp_t *p_pdp;
    struct pd_t *pd;

    ASSERT((paddr & 0xfff) == 0);	//align 4k
    ASSERT((flag & ~0xfff) == 0);
    i = (vaddr >> PML4_SHIFT) & (PML4_NR - 1);
    j = (vaddr >> PDP_SHIFT) & (PML4_PER_PDP - 1);
    k = (vaddr >> PD_SHIFT) & (PD_PER_PTE - 1);
    //printk("map_page_p2v %lx,%lx,i:%d,j:%d,k:%d\n",paddr,vaddr,i,j,k);
    p_pdp = (struct pdp_t *)(pml4.pml4e[i] & ~0xfff);
    if (!p_pdp) {

        p_pdp = (struct pdp_t *)alloc_kheap_4k(4096);
        DEBUG_PRINT("new pdp:%lx\n", p_pdp);
        ASSERT(((ulong) p_pdp & 0xfff) == 0);
        if (!p_pdp)
            return E_NOMEM;
        memset(p_pdp, 0, 4096);
        pml4.pml4e[i] = V2P((ulong) p_pdp) | PTE_P | PTE_W | PTE_U;
    } else {
        p_pdp = (struct pdp_t *)P2V(p_pdp);
    }
    //DEBUG_PRINT("p_pdp:%lx\n",p_pdp);
    pd = (struct pd_t *)(~0xfff & (p_pdp->pdpte[j]));
    if (!pd) {
        pd = (struct pd_t *)alloc_kheap_4k(4096);
        ASSERT(((ulong) pd & 0xfff) == 0);
        if (!pd)
            return E_NOMEM;
        memset(pd, 0, 4096);
        p_pdp->pdpte[j] = V2P((ulong) pd) | PTE_P | PTE_W | PTE_U;
        DEBUG_PRINT("pd:%lx\n", pd);

    } else {
        pd = (struct pd_t *)P2V(pd);
    }
    //DEBUG_PRINT("pd:%lx\n",pd);
    pd->pde[k] = paddr | flag;
    DEBUG_PRINT("map phy:%lx to vaddr:%lx,&pte:%lx,pte:%lx,%d,%d,%d\n",paddr,vaddr,&pd->pde[k],pd->pde[k],i,j,k);
    return OK;
}
void *ioremap_nocache(ulong addr,ulong size)
{
    ulong paddr=addr&~(PAGE_SIZE-1);
    size+=addr-paddr;
    for(ulong added=0;added<=size;added+=PAGE_SIZE){
 map_page_p2v(paddr, paddr + IO_MEM_BASE,
                             PTE_P | PTE_W | PTE_PWT | PTE_PCD | PTE_PS);
        paddr+=PAGE_SIZE;
    }
    return (void *)(addr+IO_MEM_BASE);
}
void init_pgtable()
{
    init_pml4();

}
void init_pgtable_ap()
{
    //write_cr3(V2P((ulong) & pml4));
}
