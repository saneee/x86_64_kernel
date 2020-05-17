#include <yaos/printk.h>
#include <types.h>
#include <asm/cpu.h>
#include <asm/pm64.h>
#include <asm/apic.h>
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
   extern u32 *lapic_base;
   extern void kheap_debug(void);
   extern void probe_apic(void);
   extern void init_pci(void);
   extern void kernel_start(void);
   extern void *ioremap_nocache(ulong addr,ulong size);
   //char *ptr;
   u64 addr;
   init_yaos();
   uart_early_init();
   printk("rsp:%lx rip:%lx\n", read_rsp(), read_rip());
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
   kheap_debug();
   init_acpi();
   addr = get_pte_with_addr((ulong) lapic_base);
   printk("\nbase:%lx,%lx\n", addr, *(ulong *) addr);
   init_lapic();
   probe_apic();
   start_aps();
   init_pci();   
   //ptr=ioremap_nocache(0xFE000000,0x10000);
   //for(int i=0;i<3000;i++)*ptr++=(char )i;
   //printk("p:%p\n",ptr);
   //ptr -= 3000; 
   //dump_mem(ptr,0x100);
   kernel_start();
   
   //dump_mem(ptr,0x100);
   //dump_mem((char *) 0xFFFFFFFF80008400,0x100);
   __deadloop(); 
}
void ap_main()
{
   u32 cpuid = cpuid_apic_id();
   struct arch_cpu * p = get_current_cpu();
   printf("ap %d starting... per_cpu:%lx\n", cpuid,p);
   p->status=1; //通知这个cpu已经启动，可以启动其他cpu 
   ASSERT(p->apic_id == cpuid);
   cli_hlt();
   __deadloop();
}
