#include <yaos/types.h>
#include <asm/bitops.h>
#include <yaos/printk.h>
#include <asm/pgtable.h>
#include <yaos/errno.h>
#include <asm/cpu.h>
#include <yaos/kheap.h>
#include <asm/pm64.h>
#include <asm/phymem.h>
#include <asm/tlbflush.h>
#include <string.h>
#if 0
#define DEBUG_PRINT printk
#else
#define DEBUG_PRINT inline_printk
#endif
#define NUMBER_4G 0x100000000UL
#define NUMBER_1G 0x40000000UL
#define PAGE_2M_MASK (0x1FFFFFUL)
#define PAGE_2M_SIZE 0x200000UL
struct pml4_t {
    u64 pml4e[PML4_PER_PDP];
};
struct pdp_t {
    u64 pdpte[PDP_PER_PD];
};
struct pd_t {

    u64 pde[PD_PER_PTE];
};
struct pde_t {
    u64 ptr[PTRS_PER_PTE];
};
ulong bp_cr3;
static struct pml4_t pml4 __attribute__ ((aligned(4096)));;
static struct pdp_t first_pdp __attribute__ ((aligned(4096)));
static struct pdp_t second_pdp __attribute__ ((aligned(4096)));
static struct pd_t first_pd __attribute__ ((aligned(4096)));
static struct pd_t second_pd __attribute__ ((aligned(4096)));
static struct pd_t third_pd __attribute__ ((aligned(4096)));
static struct pd_t fourth_pd __attribute__ ((aligned(4096)));

static struct pd_t phy_first_pd __attribute__ ((aligned(4096)));
static struct pd_t phy_second_pd __attribute__ ((aligned(4096)));
static struct pd_t phy_third_pd __attribute__ ((aligned(4096)));
static struct pd_t phy_fourth_pd __attribute__ ((aligned(4096)));

static void init_pml4()
{
    extern ulong __max_phy_mem_addr;
    int map_page_p2v(ulong paddr, ulong vaddr, ulong flag);
    int map_1g_page_p2v(ulong paddr, ulong vaddr, ulong flag);

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
    size_t len = 0;
    ulong vaddr = PHYS_VIRT_START;
    idx = (vaddr >> PML4_SHIFT) & (PML4_NR - 1);
    pml4.pml4e[idx]=K2P((ulong) & first_pdp) | PTE_P | PTE_W;
    //setup V2P map
    while(len<__max_phy_mem_addr) {
        idx = (vaddr >> PML4_SHIFT) & (PML4_NR - 1);
        if (!pml4.pml4e[idx])break;
printk("vaddr:%lx,paddr:%lx,idx:%d\n",vaddr, V2P(vaddr),idx);
        idx = (vaddr >> PDP_SHIFT) & (PML4_PER_PDP - 1);
        first_pdp.pdpte[idx] = V2P(vaddr) | PTE_P | PTE_W | PTE_PS;
printk("pdpte:%lx,%d\n",first_pdp.pdpte[idx],idx);
        vaddr += NUMBER_1G;
        len += NUMBER_1G;
    }

    idx = (PHYS_VIRT_START >> PDP_SHIFT) & (PML4_PER_PDP - 1);
    first_pdp.pdpte[idx] = K2P((ulong) & phy_first_pd) | PTE_P | PTE_W;
    if (idx++<PML4_PER_PDP-1) {
        first_pdp.pdpte[idx] = K2P((ulong) & phy_second_pd) | PTE_P | PTE_W;
    }
    if (idx++<PML4_PER_PDP-1) {
        first_pdp.pdpte[idx] = K2P((ulong) & phy_third_pd) | PTE_P | PTE_W;
    }
    if (idx++<PML4_PER_PDP-1) {
        first_pdp.pdpte[idx] = K2P((ulong) & phy_fourth_pd) | PTE_P | PTE_W;
    }

    //init KERNEL_BASE FIRST 1G or 2G
    pml4.pml4e[(KERNEL_BASE >> PML4_SHIFT) & (PML4_NR - 1)]=K2P((ulong) & second_pdp) | PTE_P | PTE_W;
    idx = (KERNEL_BASE >> PDP_SHIFT) & (PML4_PER_PDP - 1);
    second_pdp.pdpte[idx] = K2P((ulong) & first_pd) | PTE_P | PTE_W; 
    if (idx<PML4_PER_PDP-1) {
        second_pdp.pdpte[idx+1] = K2P((ulong) & second_pd) | PTE_P | PTE_W;
    } 
    for (int i = 0; i < PD_PER_PTE; i++) {
        first_pd.pde[i] = i * PAGE_SIZE_LARGE | PTE_PS | PTE_W | PTE_P;
        second_pd.pde[i] =
            (i + PD_PER_PTE) * PAGE_SIZE_LARGE | PTE_PS | PTE_W | PTE_P;
        third_pd.pde[i] =
            (i + 2 * PD_PER_PTE) * PAGE_SIZE_LARGE | PTE_PS | PTE_W | PTE_P;
        fourth_pd.pde[i] =
            (i + 3 * PD_PER_PTE) * PAGE_SIZE_LARGE | PTE_PS | PTE_W | PTE_P;
        phy_first_pd.pde[i] = i * PAGE_SIZE_LARGE | PTE_PS | PTE_W | PTE_P;
        phy_second_pd.pde[i] =
            (i + PD_PER_PTE) * PAGE_SIZE_LARGE | PTE_PS | PTE_W | PTE_P;
        phy_third_pd.pde[i] =
            (i + 2 * PD_PER_PTE) * PAGE_SIZE_LARGE | PTE_PS | PTE_W | PTE_P;
        phy_fourth_pd.pde[i] =
            (i + 3 * PD_PER_PTE) * PAGE_SIZE_LARGE | PTE_PS | PTE_W | PTE_P;

    }
    DEBUG_PRINT("__max_phy_mem_addr:%lx\n", __max_phy_mem_addr);
    /*
    if (__max_phy_mem_addr > PAGE_SIZE_LARGE) {
        ulong addr = PAGE_SIZE_LARGE;

        for (; addr + PAGE_SIZE_LARGE < __max_phy_mem_addr; addr += PAGE_SIZE_LARGE) {
            if (OK != map_page_p2v(addr, P2V(addr), PTE_PS | PTE_P | PTE_W)) {
                panic("Not enough init heap memory!");
            }
        }
    }
    */
u64 get_phy_addr(u64 vaddr);
//dump_mem(&pml4,0x1000);
//printk("write_cr3:%lx,%lx,%lx\n",K2P((ulong) & pml4),&pml4,get_phy_addr((u64)&pml4));
    write_cr3(K2P((ulong) & pml4));
printk(" read cr3\n");
    bp_cr3 = read_cr3();
printk(" new cr3:%x\n",bp_cr3);
    //flush tlb
    ulong cr4 = read_cr4();
    write_cr4(cr4 ^ cr4_pge);
    write_cr4(cr4);
//printk("cr4:%lx\n",cr4);
    if (__max_phy_mem_addr > PAGE_SIZE_LARGE) {
/*
        ulong addr = PAGE_SIZE_LARGE;
        ulong max_addr = 0xfffffffffffe0000 - KERNEL_BASE;
        max_addr = max_addr > __max_phy_mem_addr ? __max_phy_mem_addr: max_addr;
printk("max_addr:%lx\n",max_addr);
        for (; addr + PAGE_SIZE_LARGE < max_addr; addr += PAGE_SIZE_LARGE) {
            if (OK != map_page_p2v(addr, P2K(addr), PTE_PS | PTE_P | PTE_W)) {
                panic("Not enough init heap memory!");
            }
        }
*/
        ulong addr = NUMBER_1G;

        for (; addr  < __max_phy_mem_addr; addr += PAGE_SIZE_LARGE) {
            if (OK != map_page_p2v(addr, P2V(addr), PTE_PS | PTE_P | PTE_W)) {
                panic("Not enough init heap memory!");
            }
        }

    }

}
u64 build_pte_with_addr(u64 addr)
{
    ulong i, j, k, pml4base;
    struct pml4_t *p_pml4;
    struct pdp_t *p_pdp;
    struct pd_t *p_pd;

    pml4base = K2P((ulong)&pml4);//read_cr3();
    pml4base &= ~0xfff;

    DEBUG_PRINT("pml4base %lx ", pml4base);
    i = (addr >> PML4_SHIFT) & (PML4_NR - 1);
    j = (addr >> PDP_SHIFT) & (PML4_PER_PDP - 1);
    k = (addr >> PD_SHIFT) & (PD_PER_PTE - 1);
    p_pml4 = (struct pml4_t *)P2V(pml4base);
    p_pdp = (struct pdp_t *)(p_pml4->pml4e[i] & ~0xfff);
    DEBUG_PRINT(" %lx ", p_pdp);
    if (!p_pdp) {
        p_pdp = (struct pdp_t *)P2V(alloc_phy_page_small(4096));
        DEBUG_PRINT("new pdp:%lx\n", p_pdp);
        ASSERT(((ulong) p_pdp & 0xfff) == 0);
        if (!p_pdp)
            return 0;
        memset(p_pdp, 0, 4096);
        p_pml4->pml4e[i] = V2P(p_pdp) | PTE_P | PTE_W; 
    }
    else p_pdp = (struct pdp_t *)P2V(p_pdp);
    p_pd = (struct pd_t *)(p_pdp->pdpte[j] & ~0xfff);
    DEBUG_PRINT(" %lx ", p_pd);
    if (!p_pd) {
        p_pd = (struct pd_t *)P2V(alloc_phy_page_small(4096));
        ASSERT(((ulong) p_pd & 0xfff) == 0);
        if (!p_pd)
            return 0;
        memset(p_pd, 0, 4096);
        p_pdp->pdpte[j] = V2P((ulong) p_pd) | PTE_P | PTE_W;
        DEBUG_PRINT("pd:%lx\n", p_pd);
    
    }
    else p_pd = (struct pd_t *)P2V(p_pd);

    return (u64) & p_pd->pde[k];

}
u64 get_pte_with_addr(u64 addr)
{
    ulong i, j, k, pml4base;
    struct pml4_t *p_pml4;
    struct pdp_t *p_pdp;
    struct pd_t *p_pd;

    pml4base = K2P((ulong)&pml4);//read_cr3();
    pml4base &= ~0xfff;
 
    i = (addr >> PML4_SHIFT) & (PML4_NR - 1);
    j = (addr >> PDP_SHIFT) & (PML4_PER_PDP - 1);
    k = (addr >> PD_SHIFT) & (PD_PER_PTE - 1);
    DEBUG_PRINT("pml4base %lx,id:%d ", pml4base,i);

    p_pml4 = (struct pml4_t *)P2V(pml4base);
    p_pdp = (struct pdp_t *)(p_pml4->pml4e[i] & ~0xfff);
    DEBUG_PRINT(" p_pdp:%lx,id:%d ", p_pml4->pml4e[i],i);
    if (!p_pdp)
        return 0;
    else p_pdp = (struct pdp_t *)P2V(p_pdp);
    p_pd = (struct pd_t *)(p_pdp->pdpte[j] & ~0xfff);
    DEBUG_PRINT(" %lx,id:%d \n", p_pdp->pdpte[j],j);
    if (!p_pd)
        return 0;
    else p_pd = (struct pd_t *)P2V(p_pd);

    return (u64) & p_pd->pde[k];
}
int map_1g_page_p2v(ulong paddr, ulong vaddr, ulong flag)
{
    ASSERT((vaddr & (NUMBER_1G-1)) == 0);
    ASSERT((paddr & 0xfff) == 0);
    ASSERT((flag & ~0xfff) == 0);
    ulong i, j, k;
    struct pdp_t *p_pdp;
    i = (vaddr >> PML4_SHIFT) & (PML4_NR - 1);
    j = (vaddr >> PDP_SHIFT) & (PML4_PER_PDP - 1);
    k = (vaddr >> PD_SHIFT) & (PD_PER_PTE - 1);
    p_pdp = (struct pdp_t *)(pml4.pml4e[i] & ~0xfff);
    if (!p_pdp) { 

        p_pdp = (struct pdp_t *)P2V(alloc_phy_page_small(4096));
        DEBUG_PRINT("new pdp:%lx\n", p_pdp);
        ASSERT(((ulong) p_pdp & 0xfff) == 0);
        if (!p_pdp)
            return ENOMEM;
        memset(p_pdp, 0, 4096);
        pml4.pml4e[i] = V2P((ulong) p_pdp) | PTE_P | PTE_W | PTE_U; 
    } else {
        p_pdp = (struct pdp_t *)P2V(p_pdp);
    }
    //DEBUG_PRINT("p_pdp:%lx\n",p_pdp);
    p_pdp->pdpte[j] = paddr | flag;
 
    DEBUG_PRINT("map 1G page phy:%lx to vaddr:%lx,&ptpte:%lx,ptpte:%lx,%d,%d\n",paddr,vaddr,&p_pdp->pdpte[k],p_pdp->pdpte[k],i,j);
    return OK;

}
ulong map_flags_at(ulong vaddr)
{
    ulong align2Mvaddr = vaddr & ~PAGE_2M_MASK;
    ulong  *pte = (ulong *)get_pte_with_addr(align2Mvaddr);
    if (!pte) return 0;
    if (*pte & PTE_PS) {
        return *pte & ( PTE_PWT | PTE_PCD | PTE_U | PTE_W | PTE_P | PTE_PS|PTE_G);
    }
    struct pde_t *p = (struct pde_t *)(P2V(*pte) & ~0xfff);
    unsigned int k = (vaddr >> PTE_SHIFT) & (PTRS_PER_PTE - 1);
    return p->ptr[k] & ( PTE_PWT | PTE_PCD | PTE_U | PTE_W | PTE_P | PTE_PS|PTE_G);

}
ret_t map_paddr_flags_at(ulong vaddr)
{
    ret_t ret = {0,0};
    ulong align2Mvaddr = vaddr & ~PAGE_2M_MASK;
    ulong paddr;
    ulong  *pte = (ulong *)get_pte_with_addr(align2Mvaddr);
    if (!pte) return ret;
    if ((*pte & (PTE_PS|PTE_P))==(PTE_PS|PTE_P)) {
        paddr = *pte & ~0xfff;
        paddr += vaddr & PAGE_2M_MASK;
        ret.v = paddr;
        ret.e = *pte & ( PTE_PWT | PTE_PCD | PTE_U | PTE_W | PTE_P |PTE_PS| PTE_G);
        return ret; 
    }
    struct pde_t *p = (struct pde_t *)(P2V(*pte) & ~0xfff);
    unsigned int k = (vaddr >> PTE_SHIFT) & (PTRS_PER_PTE - 1);
    ret.e =  p->ptr[k] & ( PTE_PWT | PTE_PCD | PTE_U | PTE_W | PTE_P |PTE_PS|PTE_G);
    ret.v = (p->ptr[k] & ~0xfff) + (vaddr & 0xfff);
    return ret;

}
u64 get_phy_addr(u64 vaddr)
{
    ret_t ret = map_paddr_flags_at(vaddr);
    return ret.v;
}

int map_4k_page_p2v(ulong paddr, ulong vaddr, ulong flag)
{
    DEBUG_PRINT("map_4k_page_p2v:addr:%lx,vaddr:%lx,flag:%lx\n",paddr,vaddr,flag);
    int map_page_p2v(ulong paddr, ulong vaddr, ulong flag);
    ASSERT((paddr & 0xfff) == 0);       //align 4k
    ASSERT((vaddr & 0xfff) == 0);       //align 4k
    ASSERT((flag & ~0xfff) == 0);
    ulong align2Mvaddr = vaddr & ~PAGE_2M_MASK;
    ulong *pte = (ulong *)build_pte_with_addr(align2Mvaddr); 
    if (!pte) {
       return ENOMEM; 
    }
    struct pde_t * p;
    if ((*pte & (PTE_PS|PTE_P))==(PTE_PS|PTE_P)) {
        //remap 2M to 4K page
        ulong old2M_paddr = *pte & ~0xfff;
        ulong oldflag = *pte & 0xfff & ~PTE_PS;
        DEBUG_PRINT("map_4k_page_p2v remap 2M page to 4k page :%lx to vaddr:%lx,flag:%lx\n",old2M_paddr,vaddr&~PAGE_2M_MASK,oldflag);

        p = (struct pde_t *)P2V(alloc_phy_page_small(4096));
        ASSERT(((ulong) p & 0xfff) == 0);
        if (!p)
            return ENOMEM;

        for(uint n=0;n<PTRS_PER_PTE;n++) {
            p->ptr[n]=old2M_paddr | (PTE_P | PTE_W |PTE_G );//oldflag;
            old2M_paddr += PAGE_4K_SIZE;
        }
        *pte = V2P(p) | (PTE_P | PTE_W );

    } else {
        if (!(*pte & PTE_P)) {
            ASSERT((*pte&~0xfff)==0);
            *pte = alloc_phy_page_small(4096);
            ASSERT(((ulong) *pte & 0xfff) == 0);
            if (!*pte)
                return ENOMEM;
            *pte |= PTE_W|PTE_P;

        }
        p = (struct pde_t *)(P2V(*pte) & ~0xfff);
    }
    unsigned int k = (vaddr >> PTE_SHIFT) & (PTRS_PER_PTE - 1);
    p->ptr[k]=paddr | (flag & ~PTE_PS) |PTE_G;
    flush_tlb_one_page(vaddr);
    return OK;
}
int map_copy_map(ulong vaddr, ulong oldaddr,size_t size)
{
    if (!size) return OK;
    ulong vend = vaddr + size;
    while(vaddr < vend) {
        ret_t af = map_paddr_flags_at(vaddr);
        if (af.v) return EINVAL;
        ulong paddr = af.v;
        ulong flag = af.e;
        if(map_4k_page_p2v(paddr,vaddr,flag) != OK) return ENOMEM;
        vaddr += PAGE_4K_SIZE;
    }
    return OK;
}

/*map 2M page to vaddr */
int map_page_p2v(ulong paddr, ulong vaddr, ulong flag)
{
    ulong i, j, k;
    struct pdp_t *p_pdp;
    struct pd_t *pd;

    ASSERT((vaddr & (PAGE_SIZE_LARGE-1)) == 0);	//align 4k
    ASSERT((paddr & (PAGE_SIZE_LARGE-1)) == 0);
//    ASSERT((paddr & 0xfff) == 0);
    ASSERT((flag & ~0xfff) == 0);
    i = (vaddr >> PML4_SHIFT) & (PML4_NR - 1);
    j = (vaddr >> PDP_SHIFT) & (PML4_PER_PDP - 1);
    k = (vaddr >> PD_SHIFT) & (PD_PER_PTE - 1);
    //printk("map_page_p2v %lx,%lx,i:%d,j:%d,k:%d\n",paddr,vaddr,i,j,k);
    p_pdp = (struct pdp_t *)(pml4.pml4e[i] & ~0xfff);
    if (!p_pdp) {

        p_pdp = (struct pdp_t *)P2V(alloc_phy_page_small(4096));
        DEBUG_PRINT("new pdp:%lx\n", p_pdp);
        ASSERT(((ulong) p_pdp & 0xfff) == 0);
        if (!p_pdp)
            return ENOMEM;
        memset(p_pdp, 0, 4096);
        pml4.pml4e[i] = V2P((ulong) p_pdp) | PTE_P | PTE_W | PTE_U;
    } else {
        p_pdp = (struct pdp_t *)P2V(p_pdp);
    }
    //DEBUG_PRINT("p_pdp:%lx\n",p_pdp);
    pd = (struct pd_t *)((p_pdp->pdpte[j]));
    //printk("pd:%lx\n",pd);
    if (!pd) {
        pd = (struct pd_t *)P2V(alloc_phy_page_small(4096));
        ASSERT(((ulong) pd & 0xfff) == 0);
        if (!pd)
            return ENOMEM;
        memset(pd, 0, 4096);
        p_pdp->pdpte[j] = V2P((ulong) pd) | PTE_P | PTE_W | PTE_U;
        DEBUG_PRINT("pd:%lx\n", pd);

    } else {
        if ((ulong)pd & PTE_PS) {
           //1 G page maped
            ulong old1g_paddr = (ulong)pd & ~0xfff;
            ulong oldflag = (ulong)pd & 0xfff;
            DEBUG_PRINT("remap 1g page to 2M page :%lx to vaddr:%lx\n",old1g_paddr,vaddr&(NUMBER_1G-1));

            pd = (struct pd_t *)P2V(alloc_phy_page_small(4096));
            ASSERT(((ulong) pd & 0xfff) == 0);
            if (!pd)
                return ENOMEM;

            for(uint n=0;n<PD_PER_PTE;n++) {
                pd->pde[n]=old1g_paddr | oldflag;
                old1g_paddr += PAGE_SIZE_LARGE;
            }
            p_pdp->pdpte[j] = V2P((ulong) pd) | PTE_P | PTE_W | PTE_U;
            

        } else {
            pd = (struct pd_t *)(P2V(pd) & ~0xfff);
        }
    }
    //DEBUG_PRINT("pd:%lx\n",pd);
    pd->pde[k] = paddr | flag;
    DEBUG_PRINT("map phy:%lx to vaddr:%lx,&pte:%lx,pte:%lx,%d,%d,%d\n",paddr,vaddr,&pd->pde[k],pd->pde[k],i,j,k);
    flush_tlb_one_page(vaddr);
    return OK;
}
int map_p2v(ulong paddr, ulong vaddr, size_t size, ulong flag)
{
    size_t len = 0, mapsize;
    int ret;
    if (vaddr & PAGE_2M_MASK) {
      
      mapsize = PAGE_2M_SIZE-(vaddr & PAGE_2M_MASK);
      mapsize = size > mapsize ?mapsize: size;
      while (len<mapsize) {
          if ((ret = map_4k_page_p2v(paddr,vaddr,flag))!=OK) return ret;
          paddr += PAGE_4K_SIZE;
          vaddr += PAGE_4K_SIZE;
          len += PAGE_4K_SIZE;
      }
    }
    if (size-len>=PAGE_SIZE_LARGE) {
        mapsize = size-PAGE_SIZE_LARGE;
        while( len <= mapsize ) {
            if ((ret = map_page_p2v(paddr,vaddr,flag))!=OK) return ret;
            paddr+=PAGE_SIZE_LARGE;
            vaddr+=PAGE_SIZE_LARGE;
            len+=PAGE_SIZE_LARGE;
        }
    }
    while ( len < size ) {
       if ((ret = map_4k_page_p2v(paddr,vaddr,flag))!=OK) return ret;
       paddr += PAGE_4K_SIZE;
       vaddr += PAGE_4K_SIZE;
       len += PAGE_4K_SIZE;

    }
    return OK;
}
void unmap_free_at(ulong vaddr, size_t size)
{
    size_t len = 0;
    if (size ==0 ) return;
    printk("unmap_free_at:%lx,size:%lx\n",vaddr,size);
    ulong paddr;
    ulong align2Mvaddr = vaddr & ~PAGE_2M_MASK;
    ulong *pteaddr = (ulong *)get_pte_with_addr(align2Mvaddr);
    if (!pteaddr || *pteaddr==0) {
       panic("unmap free at none exist page\n");
    }
    struct pde_t * pte;
    if ((*pteaddr & (PTE_PS|PTE_P))==(PTE_PS|PTE_P)) {

        //2M page
       if (align2Mvaddr == vaddr && PAGE_SIZE_LARGE<=size) {

           free_phy_one_page(*pteaddr & ~PAGE_2M_MASK);
           *pteaddr = 0;
           flush_tlb_one_page(vaddr);

           unmap_free_at(vaddr+PAGE_SIZE_LARGE,size-PAGE_SIZE_LARGE);
           return;   
       } else {
             //remap 2M to 4K page
            ulong old2M_paddr = *pteaddr & ~0xfff;
            ulong oldflag = (*pteaddr & 0xfff)&~PTE_PS;
            DEBUG_PRINT("unmap_free_at remap 2M page to 4k page :%lx to vaddr:%lx,flag:%lx\n",old2M_paddr,vaddr&~PAGE_2M_MASK,oldflag);

            pte = (struct pde_t *)P2V(alloc_phy_page_small(4096));
            ASSERT(((ulong) pte & 0xfff) == 0);
            if (!pte) {
                printk("NO phy_page_small to remap 2M to 4K\n");
                return;
            }

            for(uint n=0;n<PTRS_PER_PTE;n++) {
                pte->ptr[n]=old2M_paddr | oldflag;
                old2M_paddr += PAGE_4K_SIZE;
            }
            *pteaddr = V2P(pte) | (PTE_P | PTE_W | PTE_U);

 
       }
    } else  pte = (struct pde_t *)P2V(((*pteaddr) & ~0xfff));
    uint k = (vaddr >> PTE_SHIFT) & (PTRS_PER_PTE - 1);
    while (len<size) {
        paddr = pte->ptr[k] & ~0xfff;
        if (!paddr) panic("unmap free at none exist page:%lx\n",vaddr);
        free_phy_page_small(paddr, PAGE_4K_SIZE);
        flush_tlb_one_page(vaddr+len);

        pte->ptr[k] = 0;
        len+=PAGE_4K_SIZE;
        if (++k>=PTRS_PER_PTE)break;

    }
    if (len<size) unmap_free_at(vaddr+len,size-len);
    return; 
    
}
int map_alloc_at(ulong vaddr, size_t size, ulong flag)
{
    size_t len = 0, mapsize;
    int ret;
    ulong paddr,mapvaddr = vaddr;
    //printk("map_alloc_at:%lx,%lx,%lx\n",vaddr,size,flag); 
    if (vaddr & PAGE_2M_MASK) {
      mapsize = PAGE_2M_SIZE-(vaddr & PAGE_2M_MASK);
      mapsize = size > mapsize ?mapsize: size;
      while (len<mapsize) {
          paddr = (ulong)(alloc_phy_page_small(4096));
          if (!paddr) {
              ret = ENOMEM;
	  } else {
              ret = map_4k_page_p2v((paddr),mapvaddr,flag);
          }
          if (ret !=OK) {
              goto freeat;
          }
          mapvaddr += PAGE_4K_SIZE;
          len += PAGE_4K_SIZE;
      }
    }
    if (size-len>=PAGE_SIZE_LARGE) {
        mapsize = size-PAGE_SIZE_LARGE;
        while( len <= mapsize ) {
            paddr = (ulong)(alloc_phy_page());
            if (!paddr) {
                ret = ENOMEM;
            } else {
                ret = map_page_p2v((paddr),mapvaddr,flag);
            }
            if (ret !=OK) {
                goto freeat;
            }

            mapvaddr+=PAGE_SIZE_LARGE;
            len+=PAGE_SIZE_LARGE;
        }
    }
    while ( len < size ) {
       paddr = (ulong)(alloc_phy_page_small(PAGE_4K_SIZE));
       if (!paddr) {
           ret = ENOMEM;
       } else {
           ret = map_4k_page_p2v(paddr,mapvaddr,flag);
       }
       if (ret !=OK) {
           goto freeat;
       }
       mapvaddr += PAGE_4K_SIZE;
       len += PAGE_4K_SIZE;

    }
    return OK;
freeat:
    printk("map_alloc_at：%lx,size:%lx, failed\n",vaddr,len);
    if (len>0) unmap_free_at(vaddr, len);
    return ret;
}

void *ioremap_nocache(ulong addr,ulong size)
{
    ulong paddr=addr&~(PAGE_SIZE_LARGE-1);
    size+=addr-paddr;
    for(ulong added=0;added<=size;added+=PAGE_SIZE_LARGE){
 map_page_p2v(paddr, paddr + IO_MEM_BASE,
                             PTE_P | PTE_W | PTE_PWT | PTE_PCD | PTE_PS);
        paddr+=PAGE_SIZE_LARGE;
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
