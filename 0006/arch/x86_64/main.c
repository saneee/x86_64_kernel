#include <yaos/printk.h>
#include <types.h>
#include <asm/cpu.h>
#include <asm/pm64.h>
#include <yaos.h>
extern void print_regs();
extern void uart_early_init();
extern void init_cpu_bp();
extern void __deadloop();
void init_bootload(u32 addr, u32 magic);
void init_cpu(u32);
void init_bss();
void init_mmu();
void init_e820();
void init_page_map();
void init_phy_mem();
void init_pgtable();
void init_pgtable_ap();
void init_idt();
void init_acpi();
void init_kheap();
void init_lapic();
void init_ioapic();
void init_cpu_bp();
void bootup_cpu_ap();
void start_aps();
void init_time();
void init_vm(ulong start, ulong size);
void init_thread_ap(void);
extern u64 get_pte_with_addr(u64 addr);
void init_yaos()
{
    char *ptr = (char *)&g_yaos;
    int i;

    for (i = 0; i < sizeof(g_yaos); i++) {
        *ptr++ = 0;
    }
}

void bp_main(u32 info_addr, u32 magic)
{
   unsigned int *p=(unsigned int *)0LL;
   init_yaos();
   uart_early_init();
   printk("%x",*p);
   init_kheap();               //set up kheap manager
   init_bss();                 //free 64k heap first
   printk("rsp:%lx,%lx\n", read_rsp(), (ulong) & the_cpu.stack.init_stack);
   init_bootload(info_addr, magic);    //setup __max_phy_addr
   printk("rsp:%lx\n", read_rsp());
   init_phy_mem();             //set up phy memory alloc bitsmap before bootload
   init_e820();                //free phy memory,free more kheap
   init_pgtable();
   init_page_map();

   init_cpu_bp();
   __deadloop(); 
}
void ap_main()
{
   printk("ap main\n");
   cli_hlt();
   __deadloop();
}
