/* zhuhanjun 2015/10/29 
per cpu struct
*/
#ifndef ARCH_X86_64_CPU_H
#define ARCH_X86_64_CPU_H 1
#include <yaos/types.h>
#include <yaos/assert.h>
#include <asm/cpufeature.h>
#include <yaos/compiler.h>
#define DR6_RESERVED        (0xFFFF0FF0)
#define X86_VENDOR_INTEL        0
#define X86_VENDOR_CYRIX        1
#define X86_VENDOR_AMD          2
#define X86_VENDOR_UMC          3
#define X86_VENDOR_CENTAUR      5
#define X86_VENDOR_TRANSMETA    7
#define X86_VENDOR_NSC          8
#define X86_VENDOR_HYGON        9
#define X86_VENDOR_ZHAOXIN      10
#define X86_VENDOR_NUM          11

#define X86_VENDOR_UNKNOWN      0xff

struct cpuinfo_x86 {
    __u8                    x86;            /* CPU family */
    __u8                    x86_vendor;     /* CPU vendor */
    __u8                    x86_model;
    __u8                    x86_stepping;
    int                     x86_tlbsize;
    __u8                    x86_virt_bits;
    __u8                    x86_phys_bits;
    /* CPUID returned core id bits: */
    __u8                    x86_coreid_bits;
    __u8                    cu_id;
    /* Max extended CPUID function supported: */
    __u32                   extended_cpuid_level;
    /* Maximum supported CPUID level, -1=no CPUID: */
    int                     cpuid_level;
        /*
         * Align to size of unsigned long because the x86_capability array
         * is passed to bitops which require the alignment. Use unnamed
         * union to enforce the array is aligned to size of unsigned long.
         */
    union {
        __u32           x86_capability[NCAPINTS + NBUGINTS];
        unsigned long   x86_capability_alignment;
    };
    char                    x86_vendor_id[16];
    char                    x86_model_id[64];
    /* in KB - valid for CPUS which support this call: */
    unsigned int            x86_cache_size;
    int                     x86_cache_alignment;    /* In bytes */
    /* Cache QoS architectural values: */
    int                     x86_cache_max_rmid;     /* max index */
    int                     x86_cache_occ_scale;    /* scale to bytes */
    int                     x86_power;
    unsigned long           loops_per_jiffy;
    /* cpuid returned max cores value: */
    u16                     x86_max_cores;
    u16                     apicid;
    u16                     initial_apicid;
    u16                     x86_clflush_size;
   /* number of cores as seen by the OS: */
    u16                     booted_cores;
    /* Physical processor id: */
    u16                     phys_proc_id;
   /* Logical processor id: */
    u16                     logical_proc_id;
    /* Core id: */
    u16                     cpu_core_id;
    u16                     cpu_die_id;
    u16                     logical_die_id;
   /* Index into per_cpu list: */
    u16                     cpu_index;
    u32                     microcode;
    /* Address space bits used by the cache internally */
    u8                      x86_cache_bits;
    unsigned                initialized : 1;
};
extern struct cpuinfo_x86 boot_cpu_data;
extern __u32                    cpu_caps_cleared[NCAPINTS + NBUGINTS];
extern __u32                    cpu_caps_set[NCAPINTS + NBUGINTS];


enum cpuid_regs_idx {
        CPUID_EAX = 0,
        CPUID_EBX,
        CPUID_ECX,
        CPUID_EDX,
};

enum {
    X86_TRAP_DE = 0,            /*  0, Divide-by-zero */
    X86_TRAP_DB,                /*  1, Debug */
    X86_TRAP_NMI,               /*  2, Non-maskable Interrupt */
    X86_TRAP_BP,                /*  3, Breakpoint */
    X86_TRAP_OF,                /*  4, Overflow */
    X86_TRAP_BR,                /*  5, Bound Range Exceeded */
    X86_TRAP_UD,                /*  6, Invalid Opcode */
    X86_TRAP_NM,                /*  7, Device Not Available */
    X86_TRAP_DF,                /*  8, Double Fault */
    X86_TRAP_OLD_MF,            /*  9, Coprocessor Segment Overrun */
    X86_TRAP_TS,                /* 10, Invalid TSS */
    X86_TRAP_NP,                /* 11, Segment Not Present */
    X86_TRAP_SS,                /* 12, Stack Segment Fault */
    X86_TRAP_GP,                /* 13, General Protection Fault */
    X86_TRAP_PF,                /* 14, Page Fault */
    X86_TRAP_SPURIOUS,          /* 15, Spurious Interrupt */
    X86_TRAP_MF,                /* 16, x87 Floating-Point Exception */
    X86_TRAP_AC,                /* 17, Alignment Check */
    X86_TRAP_MC,                /* 18, Machine Check */
    X86_TRAP_XF,                /* 19, SIMD Floating-Point Exception */
    X86_TRAP_IRET = 32,         /* 32, IRET Exception */
};

static const ulong cr0_pe = 1u << 0;
static const ulong cr0_mp = 1u << 1;
static const ulong cr0_em = 1u << 2;
static const ulong cr0_ts = 1u << 3;
static const ulong cr0_et = 1u << 4;
static const ulong cr0_ne = 1u << 5;
static const ulong cr0_wp = 1u << 16;
static const ulong cr0_am = 1u << 18;
static const ulong cr0_nw = 1u << 29;
static const ulong cr0_cd = 1u << 30;
static const ulong cr0_pg = 1u << 31;

static const ulong cr4_vme = 1u << 0;
static const ulong cr4_pvi = 1u << 1;
static const ulong cr4_tsd = 1u << 2;
static const ulong cr4_de = 1u << 3;
static const ulong cr4_pse = 1u << 4;
static const ulong cr4_pae = 1u << 5;
static const ulong cr4_mce = 1u << 6;
static const ulong cr4_pge = 1u << 7;
static const ulong cr4_pce = 1u << 8;
static const ulong cr4_osfxsr = 1u << 9;
static const ulong cr4_osxmmexcpt = 1u << 10;
static const ulong cr4_vmxe = 1u << 13;
static const ulong cr4_smxe = 1u << 14;
static const ulong cr4_fsgsbase = 1u << 16;
static const ulong cr4_pcide = 1u << 17;
static const ulong cr4_osxsave = 1u << 18;
static const ulong cr4_smep = 1u << 20;

static const ulong rflags_if = 1u << 9;

static const u32 xcr0 = 0;
static const u64 xcr0_x87 = 1u << 0;
static const u64 xcr0_sse = 1u << 1;
static const u64 xcr0_avx = 1u << 2;

struct cpu_features_type {
    bool sse3;
    bool ssse3;
    bool cmpxchg16b;
    bool sse4_1;
    bool sse4_2;
    bool x2apic;
    bool tsc_deadline;
    bool xsave;
    bool osxsave;
    bool avx;
    bool rdrand;
    bool clflush;
    bool fsgsbase;
    bool repmovsb;
    bool gbpage;
    bool invariant_tsc;
    bool kvm_clocksource;
    bool kvm_clocksource2;
    bool kvm_clocksource_stable;
    bool kvm_pv_eoi;
    bool xen_clocksource;
    bool xen_vector_callback;
    bool xen_pci;
};
extern struct cpu_features_type cpu_features;
struct cpuid_result {
    u32 a, b, c, d;
};
static inline void native_cpuid(unsigned int *eax, unsigned int *ebx,
                                unsigned int *ecx, unsigned int *edx)
{
    /* ecx is often an input as well as an output. */
    asm volatile ("cpuid":"=a" (*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
                  :"0"(*eax), "2"(*ecx)
                  :"memory");
}
#define native_cpuid_reg(reg)                                   \
static inline unsigned int native_cpuid_##reg(unsigned int op)  \
{                                                               \
        unsigned int eax = op, ebx, ecx = 0, edx;               \
                                                                \
        native_cpuid(&eax, &ebx, &ecx, &edx);                   \
                                                                \
        return reg;                                             \
}
native_cpuid_reg(eax)
native_cpuid_reg(ebx)
native_cpuid_reg(ecx)
native_cpuid_reg(edx)
static inline void cpuid(unsigned int op,
                         unsigned int *eax, unsigned int *ebx,
                         unsigned int *ecx, unsigned int *edx)
{
        *eax = op;
        *ecx = 0;
        native_cpuid(eax, ebx, ecx, edx);
}

/* Some CPUID calls want 'count' to be placed in ecx */
static inline void cpuid_count(unsigned int op, int count,
                               unsigned int *eax, unsigned int *ebx,
                               unsigned int *ecx, unsigned int *edx)
{
        *eax = op;
        *ecx = count;
        native_cpuid(eax, ebx, ecx, edx);
}
/*
 * CPUID functions returning a single datum
 */
static inline unsigned int cpuid_eax(unsigned int op)
{
        unsigned int eax, ebx, ecx, edx;

        cpuid(op, &eax, &ebx, &ecx, &edx);

        return eax;
}

static inline unsigned int cpuid_ebx(unsigned int op)
{
        unsigned int eax, ebx, ecx, edx;

        cpuid(op, &eax, &ebx, &ecx, &edx);

        return ebx;
}
static inline unsigned int cpuid_ecx(unsigned int op)
{
        unsigned int eax, ebx, ecx, edx;

        cpuid(op, &eax, &ebx, &ecx, &edx);

        return ecx;
}

static inline unsigned int cpuid_edx(unsigned int op)
{
        unsigned int eax, ebx, ecx, edx;

        cpuid(op, &eax, &ebx, &ecx, &edx);

        return edx;
}


static inline ulong read_rsp()
{
    ulong r;
    asm volatile ("mov %%rsp, %0":"=r" (r));

    return r;
}

static inline ulong read_rbp()
{
    ulong r;
    asm volatile ("mov %%rbp, %0":"=r" (r));

    return r;
}

static inline ulong read_cr0()
{
    ulong r;
    asm volatile ("mov %%cr0, %0":"=r" (r));

    return r;
}

static inline void write_cr0(ulong r)
{
    asm volatile ("mov %0, %%cr0"::"r" (r));
}

static inline ulong read_cr2()
{
    ulong r;
    asm volatile ("mov %%cr2, %0":"=r" (r));

    return r;
}

static inline void write_cr2(ulong r)
{
    asm volatile ("mov %0, %%cr2"::"r" (r));
}
extern ulong read_rip();

static inline ulong read_cr3()
{
    ulong r;
    asm volatile ("mov %%cr3, %0":"=r" (r));

    return r;
}

static inline void write_cr3(ulong r)
{
    asm volatile ("mov %0, %%cr3"::"r" (r));
}

static inline ulong read_cr4()
{
    ulong r;
    asm volatile ("mov %%cr4, %0":"=r" (r));

    return r;
}

static inline void write_cr4(ulong r)
{
    asm volatile ("mov %0, %%cr4"::"r" (r));
}

static inline ulong read_cr8()
{
    ulong r;
    asm volatile ("mov %%cr8, %0":"=r" (r));

    return r;
}

static inline void write_cr8(ulong r)
{
    asm volatile ("mov %0, %%cr8"::"r" (r));
}

struct desc_ptr {
    u16 limit;
    ulong addr;
} __attribute__ ((packed));

static inline void lgdt(struct desc_ptr *ptr)
{
    asm volatile ("lgdt %0"::"m" (*ptr));
}

static inline void sgdt(struct desc_ptr *ptr)
{
    asm volatile ("sgdt %0":"=m" (*ptr));
}

static inline void lidt(struct desc_ptr *ptr)
{
    asm volatile ("lidt %0"::"m" (*ptr));
}

static inline void sidt(struct desc_ptr *ptr)
{
    asm volatile ("sidt %0":"=m" (*ptr));
}

static inline void ltr(u16 tr)
{
    asm volatile ("ltr %0"::"rm" (tr));
}

static inline u16 read_tr()
{
    u16 tr;
    asm volatile ("str %0":"=rm" (tr));

    return tr;
}

static inline u16 read_cs()
{
    u16 r;
    asm volatile ("mov %%cs, %0":"=rm" (r));

    return r;
}

static inline u16 read_ds()
{
    u16 r;
    asm volatile ("mov %%ds, %0":"=rm" (r));

    return r;
}

static inline void write_ds(u16 r)
{
    asm volatile ("mov %0, %%ds"::"rm" (r));
}

static inline u16 read_es()
{
    u16 r;
    asm volatile ("mov %%es, %0":"=rm" (r));

    return r;
}

static inline void write_es(u16 r)
{
    asm volatile ("mov %0, %%es"::"rm" (r));
}

static inline u16 read_fs()
{
    u16 r;
    asm volatile ("mov %%fs, %0":"=rm" (r));

    return r;
}

static inline void write_fs(u16 r)
{
    asm volatile ("mov %0, %%fs"::"rm" (r));
}

static inline u16 read_gs()
{
    u16 r;
    asm volatile ("mov %%gs, %0":"=rm" (r));

    return r;
}

static inline void write_gs(u16 r)
{
    asm volatile ("mov %0, %%gs"::"rm" (r));
}

static inline u16 read_ss()
{
    u16 r;
    asm volatile ("mov %%ss, %0":"=rm" (r));

    return r;
}

static inline void write_ss(u16 r)
{
    asm volatile ("mov %0, %%ss"::"rm" (r));
}

static inline u64 rdmsr(u32 index)
{
    u32 lo, hi;
    asm volatile ("rdmsr":"=a" (lo), "=d"(hi):"c"(index));

    return lo | ((u64) hi << 32);
}

static inline void wrmsr(u32 index, u64 data)
{
    u32 lo = data, hi = data >> 32;
    asm volatile ("wrmsr"::"c" (index), "a"(lo), "d"(hi));
}

static inline bool wrmsr_safe(u32 index, u64 data)
{
    u32 lo = data, hi = data >> 32;

    bool ret = true;
    asm volatile ("1: \n\t"
                  "wrmsr\n\t"
                  "2: \n\t"
                  ".pushsection .text.fixup, \"ax\" \n\t"
                  "3: \n\t"
                  "xor %[ret], %[ret]\n\t"
                  "jmp 2b \n\t"
                  ".popsection \n\t"
                  ".pushsection .fixup, \"aw\" \n\t"
                  ".quad 1b, 3b \n\t" ".popsection\n":[ret] "+r"(ret)
                  :"c"(index), "a"(lo), "d"(hi));

    return ret;
}

static inline void wrgsbase(u64 data)
{
    asm volatile ("wrgsbase %0"::"r" (data));
}

static inline void wrfsbase(u64 data)
{
    asm volatile ("wrfsbase %0"::"r" (data));
}

static inline unsigned long rdgsbase()
{
    ulong r;
    asm volatile ("rdgsbase %0":"=r" (r));

    return r;
}

static inline unsigned long rdfsbase()
{
    ulong r;
    asm volatile ("rdfsbase %0":"=r" (r));

    return r;
}

static inline void cli_hlt()
{
    asm volatile ("cli; hlt":::"memory");
}

static inline void sti_hlt()
{
    asm volatile ("sti; hlt":::"memory");
}

static inline u8 inb(u16 port)
{
    u8 r;
    asm volatile ("inb %1, %0":"=a" (r):"dN"(port));

    return r;
}

static inline u16 inw(u16 port)
{
    u16 r;
    asm volatile ("inw %1, %0":"=a" (r):"dN"(port));

    return r;
}

static inline u32 inl(u16 port)
{
    u32 r;
    asm volatile ("inl %1, %0":"=a" (r):"dN"(port));

    return r;
}

static inline void insl(void *addr, int cnt, u16 port)
{
    asm volatile ("rep insl":"+D" (addr), "+c"(cnt)
                  :"d"(port)
                  :"memory", "cc");
}

static inline void outb(u8 val, u16 port)
{
    asm volatile ("outb %0, %1"::"a" (val), "dN"(port));

}

static inline void outw(u16 val, u16 port)
{
    asm volatile ("outw %0, %1"::"a" (val), "dN"(port));

}

static inline void outl(u32 val, u16 port)
{
    asm volatile ("outl %0, %1"::"a" (val), "dN"(port));

}

/* cnt is not bytes, 4byte counter */
static inline void outsl(void *addr, int cnt, u16 port)
{
    asm volatile ("rep outsl":"+S" (addr), "+c"(cnt)
                  :"d"(port)
                  :"cc");
}

static inline void sti()
{
    asm volatile ("sti":::"memory");
}

static inline void cli()
{
    asm volatile ("cli":::"memory");
}

__attribute__ ((no_instrument_function))
static inline void cli_notrace();

static inline void cli_notrace()
{
    asm volatile ("cli":::"memory");
}

static inline u64 rdtsc()
{

    unsigned low, high;
    asm volatile ("rdtsc":"=a" (low), "=d"(high));

    return ((low) | ((u64) (high) << 32));
}

static inline u64 ticks()
{
    return rdtsc();
}

struct fpu_state {
    char legacy[512];
    char xsavehdr[24];
    char reserved[40];
    char ymm[256];
} __attribute__ ((packed));

static inline void fxsave(struct fpu_state *s)
{
    asm volatile ("fxsaveq %0":"=m" (*s));
}

static inline void fxrstor(struct fpu_state *s)
{
    asm volatile ("fxrstorq %0"::"m" (*s));
}

static inline void xsave(struct fpu_state *s, u64 mask)
{
    u32 a = mask, d = mask >> 32;
    asm volatile ("xsaveq %[fpu]":[fpu] "=m"(*s):"a"(a), "d"(d));
}

static inline void xsaveopt(struct fpu_state *s, u64 mask)
{
    u32 a = mask, d = mask >> 32;
    asm volatile ("xsaveoptq %[fpu]":[fpu] "=m"(*s):"a"(a), "d"(d));
}

static inline void xrstor(const struct fpu_state *s, u64 mask)
{
    u32 a = mask, d = mask >> 32;
    asm volatile ("xrstorq %[fpu]"::[fpu] "m"(*s), "a"(a), "d"(d));
}

static inline void write_xcr(u32 reg, u64 val)
{
    u32 a = val;
    u32 d = val >> 32;
    asm volatile ("xsetbv"::"c" (reg), "a"(a), "d"(d));
}

static inline u64 read_xcr(u32 reg)
{
    u32 a, d;
    asm volatile ("xgetbv":"=a" (a), "=d"(d):"c"(reg));

    return (a | ((u64) d << 32));
}

static inline void init_fpu()
{
    asm volatile ("fninit":::"memory");
    unsigned int csr = 0x1f80;
    asm volatile ("ldmxcsr %0"::"m" (csr));
}

static inline void lfence()
{
    asm volatile ("lfence");
}

static inline bool rdrand(u64 * dest)
{
    unsigned char ok;
    asm volatile ("rdrand %0; setc %1":"=r" (*dest), "=qm"(ok)
                  ::"cc");

    return ok;
}

static inline uint32_t stmxcsr()
{
    uint32_t addr;
    asm volatile ("stmxcsr %0":"=m" (addr));

    return addr;
}

static inline void ldmxcsr(uint32_t addr)
{
    asm volatile ("ldmxcsr %0"::"m" (addr));
}

static inline uint16_t fnstcw()
{
    uint16_t addr;
    asm volatile ("fnstcw %0":"=m" (addr));

    return addr;
}

static inline void fldcw(uint16_t addr)
{
    asm volatile ("fldcw %0"::"m" (addr));
}

static inline unsigned int cpuid_apic_id()
{
    u32 eax = 1;
    u32 ecx = 0;
    u32 ebx, edx;

    native_cpuid(&eax, &ebx, &ecx, &edx);
    return ebx >> 24;
}

static inline unsigned long native_save_fl(void)
{
    unsigned long flags;

    /*
     * "=rm" is safe here, because "pop" adjusts the stack before
     * it evaluates its effective address -- this is part of the
     * documented behavior of the "pop" instruction.
     */
    asm volatile ("# __raw_save_flags\n\t" "pushf ; pop %0":"=rm" (flags)
                  :             /* no input */
                  :"memory");

    return flags;
}

static inline void native_restore_fl(unsigned long flags)
{
    asm volatile ("push %0 ; popf":	/* no output */
                  :"g" (flags)
                  :"memory", "cc");
}

static inline void native_irq_disable(void)
{
    asm volatile ("cli":::"memory");
}

static inline void native_irq_enable(void)
{
    asm volatile ("sti":::"memory");
}

static inline notrace unsigned long arch_local_save_flags(void)
{
    return native_save_fl();
}

static inline notrace void arch_local_irq_restore(unsigned long flags)
{
    native_restore_fl(flags);
}

static inline notrace void arch_local_irq_disable(void)
{
    native_irq_disable();
}

static inline notrace void arch_local_irq_enable(void)
{
    native_irq_enable();
}

static inline notrace unsigned long arch_local_irq_save(void)
{
    unsigned long flags = arch_local_save_flags();

    arch_local_irq_disable();
    return flags;
}
static inline unsigned long native_get_debugreg(int regno)
{
        unsigned long val = 0;  /* Damn you, gcc! */

        switch (regno) {
        case 0:
                asm("mov %%db0, %0" :"=r" (val));
                break;
        case 1:
                asm("mov %%db1, %0" :"=r" (val));
                break;
        case 2:
                asm("mov %%db2, %0" :"=r" (val));
                break;
        case 3:
                asm("mov %%db3, %0" :"=r" (val));
                break;
        case 6:
                asm("mov %%db6, %0" :"=r" (val));
                break;
        case 7:
                asm("mov %%db7, %0" :"=r" (val));
                break;
        default:
                BUG();
        }
        return val;
}
#define get_debugreg(var, register) (var) = native_get_debugreg(register)
static inline void sync_core(void)
{
    int tmp;
    asm volatile ("cpuid":"=a" (tmp)
                  :"0"(1)
                  :"ebx", "ecx", "edx", "memory");
}

static inline unsigned long get_gs_base()
{
#ifdef __FSGSBASE__
    return rdgsbase();
#else
    if (cpu_features.fsgsbase)
        return rdgsbase();
    else {
        u32 lo, hi;
        u32 index = 0xc0000101;
        asm volatile ("rdmsr":"=a" (lo), "=d"(hi):"c"(index));

        return lo | ((u64) hi << 32);

    }
#endif
}

static inline void set_gs_base(ulong base)
{
#ifdef __FSGSBASE__
    wrgsbase(base);
#else
    if (cpu_features.fsgsbase)
        return wrgsbase(base);
    else {
        wrmsr(0xc0000101, base);

    }
#endif

}

void print_regs();

#endif
