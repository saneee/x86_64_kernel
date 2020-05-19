/* zhuhanjun 2015/10/29 
*/
#include <asm/cpu.h>
#include <asm/pm64.h>
#include <string.h>
#include <yaos/assert.h>
#include <yaos/printk.h>
#include <yaos/percpu.h>
#include <asm/apic.h>
#include <asm/pgtable.h>
#include <yaos/barrier.h>
#include <yaos/cpupm.h>
#include <yaos/init.h>
#include <yaos/cache.h>
#include <asm/irq.h>
#define MAX_CPUS	1024
#if 1
#define DEBUG_PRINTK printk
#else
#define DEBUG_PRINTK inline_printk
#endif
extern ulong bp_cr3;
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

DEFINE_PER_CPU_SHARED_ALIGNED(irq_cpustat_t, irq_stat);

DEFINE_PER_CPU_FIRST(struct cpu, the_cpu);
DEFINE_PER_CPU(struct init_stack, init_stack);
DEFINE_PER_CPU(struct task_struct *, current_task) ____cacheline_aligned;
DEFINE_PER_CPU(char *, irq_stack_ptr);

DEFINE_PER_CPU(unsigned int, irq_count) __visible = -1;
DEFINE_PER_CPU(unsigned int, tasklet_count) __visible = -1;

DEFINE_PER_CPU(struct orig_ist, orig_ist);
DEFINE_PER_CPU(char *, current_thread);
static const unsigned int exception_stack_sizes[N_EXCEPTION_STACKS + 1] = {
    [0 ... N_EXCEPTION_STACKS] = EXCEPTION_STKSZ,
    [DEBUG_STACK] = DEBUG_STKSZ,
    [IRQ_STACK] = IRQ_STACK_SIZE
};

static DEFINE_PER_CPU_PAGE_ALIGNED(char, exception_stacks
                                   [(N_EXCEPTION_STACKS - 2) * EXCEPTION_STKSZ +
                                    DEBUG_STKSZ + IRQ_STACK_SIZE]);

static DEFINE_PER_CPU(unsigned long, debug_stack_addr);

DEFINE_PER_CPU_READ_MOSTLY(unsigned long, this_cpu_off) =
    ((unsigned long)__per_cpu_load);
DEFINE_PER_CPU_READ_MOSTLY(int, cpu_number);

char *cpu_base[MAX_CPUS];
extern int nr_cpu;
void test()
{
    print_regs();
    printk("jumped to zero?\n");
    for (;;) ;
}
static cpu_p apic_id_to_cpu(u32 apicid)
{
    cpu_p pcpu;
    for (int i = 0; i < nr_cpu; i++) {
        pcpu = base_to_cpu(cpu_base[i]);
        if (pcpu->arch_cpu.apic_id ==apicid) return pcpu;
    }
    return (cpu_p) 0;
}

static inline void init_ist(cpu_p p)
{
    char *estacks = (char *)PER_CPU_PTR(&exception_stacks,p);

    struct orig_ist *oist = PER_CPU_PTR(&orig_ist,p); 
printk("estack:%p,oist:%p,%d,%lx\n",estacks,oist,sizeof(exception_stack_sizes),sizeof(exception_stacks));
    for (int i = 0; i < 8; i++)
        oist->ist[i] = 0;
    p->arch_cpu.atss.tss.ist[0] = 0;
    for (int v = 1; v < N_EXCEPTION_STACKS + 1; v++) {
        estacks += exception_stack_sizes[v];
        oist->ist[v] = p->arch_cpu.atss.tss.ist[v] = (unsigned long)estacks;
        if (v == DEBUG_STACK) {
            PER_CPU_PTR_VAR(&debug_stack_addr,p) = (unsigned long)estacks;
        }
        else if (v == IRQ_STACK) {
            /*top 64 used in ist:ss,rsp,flag,cs,ip,errno etc */
            PER_CPU_PTR_VAR(&irq_stack_ptr,p) = estacks - 64;

        }

    }

}
static inline void init_percpu(cpu_p p)
{
    int cpu = p->cpu;

    per_cpu(irq_count, cpu) = -1;
    per_cpu(tasklet_count, cpu) = -1;

    per_cpu(this_cpu_off, cpu) = (unsigned long)p->percpu_base;
    per_cpu(cpu_number, cpu) = cpu;
    irq_cpustat_t *pstat = per_cpu_ptr(&irq_stat, cpu);

    memset(pstat, 0, sizeof(irq_stat));
}

void set_int_vector(unsigned n, unsigned ist, void *handler)
{
    ulong addr = (ulong) handler;
    struct idt_entry e;

    ASSERT(n < 256);
    e.offset0 = addr;
    e.selector = read_cs();
    // We can't take interrupts on the main stack due to the x86-64 redzone
    e.ist = ist;
    e.type = INTR_GATE_TYPE;
    e.s = 0;
    e.dpl = 0;
    e.p = 1;
    e.offset1 = addr >> 16;
    e.offset2 = addr >> 32;
    _idt[n] = e;
}

void init_arch_cpu(cpu_p cpu)
{
    struct arch_cpu *p = &cpu->arch_cpu;
    p->status = 0;
    memset(&p->atss, 0, sizeof(p->atss));

    p->gdt[0] = 0;
    p->gdt[gdt_cs] = 0x00af9b000000ffff;
    p->gdt[gdt_ds] = 0x00cf93000000ffff;
    p->gdt[gdt_cs32] = 0x00cf9b000000ffff;
    p->gdt[gdt_tss] = 0x0000890000000067;
    u64 tss_addr = (u64) & p->atss.tss;
    p->gdt[gdt_tss] |= (tss_addr & 0x00ffffff) << 16;
    p->gdt[gdt_tss] |= (tss_addr & 0xff000000) << 32;
    p->gdt[gdt_tssx] = tss_addr >> 32;
    init_ist(cpu);

}

void bootup_cpu(cpu_p p)
{
    extern void probe_apic();
    void init_idt(void);
    struct desc_ptr desc;
    ulong gsbase = (ulong) p->percpu_base;

    ASSERT(gsbase);
    DEBUG_PRINTK("starting %d,gsbase:%lx,cr4:%lx,p:%lx\n", p->cpu,
                 gsbase, read_cr4(), p);
    desc.limit = sizeof(_idt) - 1;
    desc.addr = (ulong) & _idt;
    lidt(&desc);
    desc.limit = nr_gdt * 8 - 1;
    desc.addr = (ulong) (&p->arch_cpu.gdt);


    lgdt(&desc);
    write_ds(gdt_ds * 8);
    write_es(gdt_ds * 8);
    write_ss(gdt_ds * 8);
    write_fs(gdt_ds * 8);
    write_gs(gdt_ds * 8);

    ltr(gdt_tss * 8);
    barrier();

    set_gs_base(gsbase);

    p->status = CPU_RUNNING;

}

void bootup_cpu_ap()
{
    cpu_p pcpu = apic_id_to_cpu(cpuid_apic_id());
    if (!pcpu)panic("can't found ap pcpu");

    bootup_cpu(pcpu);
}

void bootup_cpu_bp()
{
    void init_idt(void);
    init_idt();
    bootup_cpu(&INIT_PER_CPU_VAR(the_cpu));

}

void __init jmp_zero_hook()
{
    uchar code[20];
    ulong addr = (ulong) test - 5;
    uchar *ptr = (uchar *) 0;

    code[0] = 0xe9;
    code[1] = addr & 0xff;
    code[2] = (addr >> 8) & 0xff;
    code[3] = (addr >> 16) & 0xff;
    code[4] = (addr >> 24) & 0xff;
    for (int i = 0; i < 5; i++) {
        *ptr++ = code[i];
        printf(" %x ", code[i]);
    }
    DEBUG_PRINTK("test %lx\n", (ulong) test - 5);
}

void __init init_cpu_bp(void)
{
    cpu_p pcpu = &INIT_PER_CPU_VAR(the_cpu);
    pcpu->cpu = 0;
    pcpu->arch_cpu.apic_id = cpuid_apic_id();
    pcpu->percpu_base = (char *)__per_cpu_load;
    cpu_base[0] = (char *)__per_cpu_load;

    DEBUG_PRINTK("bp base:%lx,%lx\n", pcpu, pcpu->percpu_base);
    init_arch_cpu(pcpu);
    //jmp_zero_hook();
}

void init_cpu(u32 apicid)
{
    cpu_p pcpu = &INIT_PER_CPU_VAR(the_cpu);

    cpu_p p;
    char *ptr;
    void init_idt();
    if (pcpu->arch_cpu.apic_id == apicid) {
        nr_cpu++;
        init_cpu_bp();
	return; //skip bp
    }
    ptr = new_percpu();

    if (apicid > MAX_CPUS) {
        panic("apicid:%lx > MAX_CPUS:%lx\n", apicid, MAX_CPUS);
    }
    if (!ptr)
        panic("No memory");
    cpu_base[nr_cpu] = ptr;
    p = base_to_cpu(ptr);
    p->percpu_base = ptr;
    p->arch_cpu.apic_id = apicid;
    p->cpu = nr_cpu++;
    init_percpu(p);
    DEBUG_PRINTK("cpu:%x,%lx,%d,%lx\n", nr_cpu, p, p->arch_cpu.apic_id, ptr);
    init_arch_cpu(p);
}

static inline void set_intr_gate(int v, int ist)
{
    extern ulong vectors[];

    set_int_vector(v, ist, (char *)vectors[v]);

}

void init_idt()
{
    struct desc_ptr desc;
    extern ulong vectors[];

    for (int i = 0; i < 32; i++) {
        set_int_vector(i, DEFAULT_EXCEPT_STACK, (char *)vectors[i]);
    }
    for (int i = 32; i < 0x80; i++) {
        set_int_vector(i, IRQ_STACK, (char *)vectors[i]);
    }
    set_int_vector(0x80, 0, (char *)vectors[0x80]);
    for (int i = 0x81; i < 256; i++) {
        set_int_vector(i, IRQ_STACK, (char *)vectors[i]);
    }
    set_intr_gate(0x80, 0);
    set_intr_gate(X86_TRAP_NMI, NMI_STACK);
    set_intr_gate(X86_TRAP_DF, DOUBLEFAULT_STACK);
    set_intr_gate(X86_TRAP_MC, MCE_STACK);
    set_intr_gate(X86_TRAP_DB, DEBUG_STACK);
    set_intr_gate(X86_TRAP_BP, DEBUG_STACK);

    desc.limit = sizeof(_idt) - 1;
    desc.addr = (ulong) & _idt;
    lidt(&desc);
    DEBUG_PRINTK("sizeof(_idt):%lx,%lx,%lx\n", sizeof(_idt), desc.addr,
                 desc.limit);
}

void init_ap_early()
{
    write_cr3(bp_cr3);
}
void bootup_aps(void)
{
    extern uchar _binary_out_entryother_start[], _binary_out_entryother_size[];
    extern void lapic_start_ap(uint apicid, uint addr);
    extern void entry32mp(void);
    uchar *code;
    __thread cpu_p p;
    u32 apicid = cpuid_apic_id();

    code = (uchar *) P2V(0x7000);
    memmove(code, _binary_out_entryother_start,
            (uintp) _binary_out_entryother_size);
    DEBUG_PRINTK("bin start:%lx,size:%lx\n", _binary_out_entryother_start,
                 _binary_out_entryother_size);
    for (int i = 0; i < nr_cpu; i++) {
        p = base_to_cpu(cpu_base[i]);
        DEBUG_PRINTK("try %d,%lx,%d\n", p->arch_cpu.apic_id, p, nr_cpu);
        if (p->arch_cpu.apic_id == apicid) {
            continue;
        }
        p->status = 0;
        *(uint32 *) (code - 4) = 0x7000 - 32;	//init stack in code 16,code 32
        *(uint32 *) (code - 8) = V2P((u64) entry32mp);
        //init stack in code 64
        *(uint64 *) (code - 16) =
            (uint64) per_cpu_ptr(&init_stack, i) + sizeof(init_stack);

        DEBUG_PRINTK("startcode:%lx,stack:%lx,cr3:%lx\n",
                     V2P((u64) entry32mp),
                     (uint64) per_cpu_ptr(&init_stack, i) + sizeof(init_stack),
                     read_cr3());

        lapic_start_ap(p->arch_cpu.apic_id, 0x7000);
        DEBUG_PRINTK("wait for %d,%lx\n", p->arch_cpu.apic_id, &p->status);

        barrier();

        while (p->status == 0) ;

    }

}
