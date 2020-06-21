#include <asm/cpu.h>
#include <yaos/percpu.h>
#include <asm/pm64.h>
#include <yaos.h>
#include <yaos/printk.h>
#include <yaos/kheap.h>
#include <asm/pgtable.h>
#include <asm/cpu.h>
#include <asm/apic.h>
#include <asm/irq.h>
extern void uart_early_init();
extern void bzero(void *s, size_t n);
void start_kernel();            //kernel/main.c
void start_kernel_ap();         //kernel/main.c

//extern void mi_startup(void);
//extern void virtio_setup_intr(void);
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
void init_phy_page();
void init_lapic();
void init_ioapic();
void init_cpu_bp();
void bootup_cpu_ap();
void init_time();
void init_vm(ulong start, ulong size);
void init_thread_ap(void);
void test_base()
{
    extern char _module_data_start[];
    extern char _module_data_end[];

    printk("module_ptr_start:%lx,end:%lx\n", _module_data_start,
           _module_data_end);
}

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
    extern void init_cpuid();
    extern void alternative_instructions(void);
    extern void probe_apic();
    extern void init_pci();
    extern void bootup_cpu_bp();
    extern void __memcpy(void *d,void *s,int size);
    init_yaos();
    uart_early_init();
    init_cpuid();
    alternative_instructions();
    init_kheap();               //set up kheap manager
    init_phy_page();
    init_bss();                 //free 64k heap first
    printk("rsp:%lx\n", read_rsp());
    init_bootload(info_addr, magic);	//setup __max_phy_addr
    printk("rsp:%lx\n", read_rsp());
    init_phy_mem();             //set up phy memory alloc bitsmap before bootload
    init_e820();                //free phy memory,free more kheap
    init_pgtable();

    init_page_map();
    //init_cpu_bp();              //load bp seg
    init_acpi();
    init_lapic();
    init_time();
    init_ioapic();
    init_vm(VMALLOC_START, VMALLOC_SIZE);

    bootup_cpu_bp();
    init_pci();
    start_kernel();             //bp start
}

void arch_setup(void)           //call from kernel/main.c
{
}


void ap_main(void)
{

    printf("ap starting...%d\n", cpuid_apic_id());
    void init_ap_early();
    extern void init_cpuid_ap();

    init_ap_early();
    init_cpuid_ap();
    init_pgtable_ap();
    init_lapic();
    bootup_cpu_ap();
    start_kernel_ap();
}
