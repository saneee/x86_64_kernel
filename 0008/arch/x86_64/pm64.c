#define MAX_CPUS        1024
#include <asm/cpu.h>
#include <asm/pm64.h>
#include <yaos/percpu.h>
#include <yaos/string.h>
#include <asm/pgtable.h>
#include <asm/current.h>
uint nr_cpu = 0;

struct thread_t;
//cpu数据，每个cpu各自一份，主cpu使用the_cpu，其他cpu会复制.percpu段。
DEFINE_PERCPU_FIRST(struct arch_cpu, __the_cpu);
DEFINE_PERCPU(struct desc_ptr, the_desc);
DEFINE_PERCPU(struct thread_t *, current_thread);
DECLARE_INIT_PER_CPU(current_thread);
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
void init_arch_cpu(struct arch_cpu *p);

void init_cpu(u32 apicid)
{
    struct arch_cpu *p;
    char *ptr;
    void init_idt();

    if (!nr_cpu) {              //first cpu
        ptr = (char *)&the_cpu;
    }
    else {
        ptr = new_percpu();
    }
    if(apicid>MAX_CPUS){
       panic("apicid:%lx > MAX_CPUS:%lx\n",apicid,MAX_CPUS);
    }
    nr_cpu++;
    cpu_base[apicid] = ptr;
    p = base_to_cpu(ptr);
    p->percpu_base = ptr;
    p->apic_id = apicid;

    printk("cpu:%x,%lx,%d\n", nr_cpu, p, p->apic_id);
    init_arch_cpu(p);

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


/*主cpu启动*/
void bootup_cpu_bp()
{
   extern void install_zero_hook();
   //install_zero_hook(); //地址0跳转hook
   printk("install zero hook\n");
   bootup_cpu(&the_cpu);
}
void init_cpu_bp(void)
{
    init_arch_cpu(&the_cpu);
    printk("bootup_cpu_bp\n");
    bootup_cpu_bp();
}
void start_aps(void)
{
    extern uchar _binary_out_entryother_start[], _binary_out_entryother_size[];
    extern void lapic_start_ap(uint apicid, uint addr);
    extern void entry32mp(void);
    uchar *code;

    __thread struct arch_cpu *p;
    u32 apicid = cpuid_apic_id();
    code = (uchar *) P2V(0x7000);
    memmove(code, _binary_out_entryother_start,
            (uintp) _binary_out_entryother_size);
    dump_mem(code-16,0x100);
    for (int i = 0; i < nr_cpu; i++) {
        p = base_to_cpu(cpu_base[i]);
        printk("try %d,%lx,%d\n", p->apic_id, p, nr_cpu);
        if (p->apic_id == apicid) {
            continue;
        }
        *(uint32 *) (code - 4) = 0x7000 - 32;   //init stack in code 16,code 32
        *(uint32 *) (code - 8) = V2P((u64) entry32mp);
        //init stack in code 64
        *(uint64 *) (code - 16) =
            ((uint64) (&p->stack.init_stack)) + INIT_STACK_SIZE;
        //dump_mem(code-16,0x100);
        printk("startcode:%x,stack:%lx,cr3:%lx\n",
               V2P((u64) entry32mp),
               ((uint64) (&p->stack.init_stack)) + INIT_STACK_SIZE, read_cr3());
        lapic_start_ap(p->apic_id, 0x7000);
        printk("wait for %d,%lx\n", p->apic_id, &p->status);
        while (p->status == 0) ;
        //dump_mem(code-16,0x100);


    }
}
