#define MAX_CPUS        1024
#include <asm/cpu.h>
#include <asm/pm64.h>
#include <yaos/percpu.h>
#include <yaos/string.h>
//cpu数据，每个cpu各自一份，主cpu使用the_cpu，其他cpu会复制.percpu段。
PERCPU(struct arch_cpu, the_cpu);
char *cpu_base[MAX_CPUS]; //每个cpu的隐私数据起始地址。

struct idt_entry {
    u16 offset0;
    u16 selector;
    u8 ist:3;
    u8 res0:5;
    u8 type:4;
    u8 s:1;
    u8 dpl:2;
    u8 p:1;
    u16 offset1;
    u32 offset2;
    u32 res1;
} __attribute__ ((aligned(16)));
struct idt_entry _idt[256];
void add_int_vector(unsigned n, unsigned ist, void *handler)
{
    ulong addr = (ulong) handler;
    struct idt_entry e;

    ASSERT(n < 256);
    e.offset0 = addr;
    e.selector = read_cs();
    e.ist = ist;
    e.type = INTR_GATE_TYPE;
    e.s = 0;
    e.dpl = 0;
    e.p = 1;
    e.offset1 = addr >> 16;
    e.offset2 = addr >> 32;
    _idt[n] = e;
}
void init_idt()
{
    struct desc_ptr desc;
    extern ulong vectors[];

    for (int i = 0; i < 32; i++) {
        //使用1号堆栈
        add_int_vector(i, 1, (char *)vectors[i]);
    }
    for (int i = 32; i < 256; i++) {
        //使用2号堆栈
        add_int_vector(i, 2, (char *)vectors[i]);
    }
    desc.limit = sizeof(_idt) - 1;
    desc.addr = (ulong) & _idt;
    lidt(&desc);
    printk("sizeof(_idt):%lx,%lx,%lx\n", sizeof(_idt), desc.addr, desc.limit);
}
void init_arch_cpu(struct arch_cpu *p)
{
    p->status = 0;
    memset(&p->atss, 0, sizeof(p->atss));
    p->gdt[0] = 0;
    p->gdt[gdt_cs] = 0x00af9b000000ffff;
    p->gdt[gdt_ds] = 0x00cf93000000ffff;
    p->gdt[gdt_cs32] = 0x00cf9b000000ffff;
    p->gdt[gdt_tss] = 0x0000890000000067;
    p->gdt[nr_gdt] = 0;
    u64 tss_addr = (u64) & p->atss.tss;

    p->gdt[gdt_tss] |= (tss_addr & 0x00ffffff) << 16;
    p->gdt[gdt_tss] |= (tss_addr & 0xff000000) << 32;
    p->gdt[gdt_tssx] = tss_addr >> 32;

    set_exception_stack(p);
    set_interrupt_stack(p);
}

void bootup_cpu(struct arch_cpu *p)
{
    extern void probe_apic();
    struct desc_ptr desc;

    printk("starting %d\n", p->apic_id);
    desc.limit = nr_gdt * 8 - 1;
    desc.addr = (ulong) (&p->gdt);
    init_idt();
    lgdt(&desc);
    ltr(gdt_tss * 8);
    p->status = CPU_RUNNING;
    printk("cpu %d started,%lx\n", p->apic_id, &p->status);
   
}
void test()
{
    print_regs();
    printk("jumped to zero?\n");
    for (;;) ;
}


/*主cpu启动*/
void bootup_cpu_bp()
{
   extern void install_zero_hook();
   install_zero_hook(); //地址0跳转hook
   printk("install zero hook\n");
   bootup_cpu(&the_cpu);
}
void init_cpu_bp(void)
{
    init_arch_cpu(&the_cpu);
    printk("bootup_cpu_bp\n");
    bootup_cpu_bp();
}
