#include <yaos/assert.h>
#include <asm/cpu.h>
#include <string.h>
#include <types.h>
#include <yaos/printk.h>
#include <asm/bitops.h>
#include <asm/pm64.h>
#include <asm/msrdef.h>
#include <yaos/spinlock.h>
#include <yaos/irq.h>
#include <yaos/init.h>
#include <asm/cpufeature.h>
#include <asm/pgtable.h>
struct cpu_features_type cpu_features;
struct cpuinfo_x86 boot_cpu_data;
/* Aligned to unsigned long to avoid split lock in atomic bitmap ops */
__u32 cpu_caps_cleared[NCAPINTS + NBUGINTS] __aligned(sizeof(unsigned long));
__u32 cpu_caps_set[NCAPINTS + NBUGINTS] __aligned(sizeof(unsigned long));

struct cpuid_bit {
        u16 feature;
        u8 reg;
        u8 bit;
        u32 level;
        u32 sub_leaf;
};

static const struct cpuid_bit cpuid_bits[] = {
        { X86_FEATURE_APERFMPERF,       CPUID_ECX,  0, 0x00000006, 0 },
        { X86_FEATURE_EPB,              CPUID_ECX,  3, 0x00000006, 0 },
        { X86_FEATURE_CQM_LLC,          CPUID_EDX,  1, 0x0000000f, 0 },
        { X86_FEATURE_CQM_OCCUP_LLC,    CPUID_EDX,  0, 0x0000000f, 1 },
        { X86_FEATURE_CQM_MBM_TOTAL,    CPUID_EDX,  1, 0x0000000f, 1 },
        { X86_FEATURE_CQM_MBM_LOCAL,    CPUID_EDX,  2, 0x0000000f, 1 },
        { X86_FEATURE_CAT_L3,           CPUID_EBX,  1, 0x00000010, 0 },
        { X86_FEATURE_CAT_L2,           CPUID_EBX,  2, 0x00000010, 0 },
        { X86_FEATURE_CDP_L3,           CPUID_ECX,  2, 0x00000010, 1 },
        { X86_FEATURE_CDP_L2,           CPUID_ECX,  2, 0x00000010, 2 },
        { X86_FEATURE_MBA,              CPUID_EBX,  3, 0x00000010, 0 },
        { X86_FEATURE_HW_PSTATE,        CPUID_EDX,  7, 0x80000007, 0 },
        { X86_FEATURE_CPB,              CPUID_EDX,  9, 0x80000007, 0 },
        { X86_FEATURE_PROC_FEEDBACK,    CPUID_EDX, 11, 0x80000007, 0 },
        { X86_FEATURE_MBA,              CPUID_EBX,  6, 0x80000008, 0 },
        { X86_FEATURE_SME,              CPUID_EAX,  0, 0x8000001f, 0 },
        { X86_FEATURE_SEV,              CPUID_EAX,  1, 0x8000001f, 0 },
        { 0, 0, 0, 0, 0 }
};

unsigned int x86_family(unsigned int sig)
{
        unsigned int x86;

        x86 = (sig >> 8) & 0xf;

        if (x86 == 0xf)
                x86 += (sig >> 20) & 0xff;

        return x86;
}
unsigned int x86_model(unsigned int sig)
{
        unsigned int fam, model;

        fam = x86_family(sig);

        model = (sig >> 4) & 0xf;

        if (fam >= 0x6)
                model += ((sig >> 16) & 0xf) << 4;

        return model;
}
unsigned int x86_stepping(unsigned int sig)
{
        return sig & 0xf;
}

void cpu_detect(struct cpuinfo_x86 *c)
{
    /* Get vendor name */
    cpuid(0x00000000, (unsigned int *)&c->cpuid_level,
        (unsigned int *)&c->x86_vendor_id[0],
        (unsigned int *)&c->x86_vendor_id[8],
        (unsigned int *)&c->x86_vendor_id[4]);

    c->x86 = 4;
    /* Intel-defined flags: level 0x00000001 */
    if (c->cpuid_level >= 0x00000001) {
        u32 junk, tfms, cap0, misc;

        cpuid(0x00000001, &tfms, &misc, &junk, &cap0);
        c->x86          = x86_family(tfms);
        c->x86_model    = x86_model(tfms);
        c->x86_stepping = x86_stepping(tfms);

        if (cap0 & (1<<19)) {
           c->x86_clflush_size = ((misc >> 8) & 0xff) * 8;
           c->x86_cache_alignment = c->x86_clflush_size;
        }
   }
}
static void init_cqm(struct cpuinfo_x86 *c)
{
    if (!cpu_has(c, X86_FEATURE_CQM_LLC)) {
        c->x86_cache_max_rmid  = -1;
        c->x86_cache_occ_scale = -1;
        return;
    }

    /* will be overridden if occupancy monitoring exists */
    c->x86_cache_max_rmid = cpuid_ebx(0xf);

    if (cpu_has(c, X86_FEATURE_CQM_OCCUP_LLC) ||
        cpu_has(c, X86_FEATURE_CQM_MBM_TOTAL) ||
        cpu_has(c, X86_FEATURE_CQM_MBM_LOCAL)) {
        u32 eax, ebx, ecx, edx;

        /* QoS sub-leaf, EAX=0Fh, ECX=1 */
        cpuid_count(0xf, 1, &eax, &ebx, &ecx, &edx);

        c->x86_cache_max_rmid  = ecx;
        c->x86_cache_occ_scale = ebx;
    }
}

void init_scattered_cpuid_features(struct cpuinfo_x86 *c)
{
        u32 max_level;
        u32 regs[4];
        const struct cpuid_bit *cb;

        for (cb = cpuid_bits; cb->feature; cb++) {

                /* Verify that the level is valid */
                max_level = cpuid_eax(cb->level & 0xffff0000);
                if (max_level < cb->level ||
                    max_level > (cb->level | 0xffff))
                        continue;

                cpuid_count(cb->level, cb->sub_leaf, &regs[CPUID_EAX],
                            &regs[CPUID_EBX], &regs[CPUID_ECX],
                            &regs[CPUID_EDX]);

                if (regs[cb->reg] & (1 << cb->bit))
                        set_cpu_cap(c, cb->feature);
        }
}
static void init_speculation_control(struct cpuinfo_x86 *c)
{
        /*
         * The Intel SPEC_CTRL CPUID bit implies IBRS and IBPB support,
         * and they also have a different bit for STIBP support. Also,
         * a hypervisor might have set the individual AMD bits even on
         * Intel CPUs, for finer-grained selection of what's available.
         */
        if (cpu_has(c, X86_FEATURE_SPEC_CTRL)) {
                set_cpu_cap(c, X86_FEATURE_IBRS);
                set_cpu_cap(c, X86_FEATURE_IBPB);
                set_cpu_cap(c, X86_FEATURE_MSR_SPEC_CTRL);
        }

        if (cpu_has(c, X86_FEATURE_INTEL_STIBP))
                set_cpu_cap(c, X86_FEATURE_STIBP);

        if (cpu_has(c, X86_FEATURE_SPEC_CTRL_SSBD) ||
            cpu_has(c, X86_FEATURE_VIRT_SSBD))
                set_cpu_cap(c, X86_FEATURE_SSBD);


        if (cpu_has(c, X86_FEATURE_AMD_IBRS)) {
                set_cpu_cap(c, X86_FEATURE_IBRS);
                set_cpu_cap(c, X86_FEATURE_MSR_SPEC_CTRL);
        }

        if (cpu_has(c, X86_FEATURE_AMD_IBPB))
                set_cpu_cap(c, X86_FEATURE_IBPB);

        if (cpu_has(c, X86_FEATURE_AMD_STIBP)) {
                set_cpu_cap(c, X86_FEATURE_STIBP);
                set_cpu_cap(c, X86_FEATURE_MSR_SPEC_CTRL);
        }

        if (cpu_has(c, X86_FEATURE_AMD_SSBD)) {
                set_cpu_cap(c, X86_FEATURE_SSBD);
                set_cpu_cap(c, X86_FEATURE_MSR_SPEC_CTRL);
                clear_cpu_cap(c, X86_FEATURE_VIRT_SSBD);
        }
}

static void apply_forced_caps(struct cpuinfo_x86 *c)
{
        int i;

        for (i = 0; i < NCAPINTS + NBUGINTS; i++) {
                c->x86_capability[i] &= ~cpu_caps_cleared[i];
                c->x86_capability[i] |= cpu_caps_set[i];
        }
}

void get_cpu_cap(struct cpuinfo_x86 *c)
{
        u32 eax, ebx, ecx, edx;

        /* Intel-defined flags: level 0x00000001 */
        if (c->cpuid_level >= 0x00000001) {
                cpuid(0x00000001, &eax, &ebx, &ecx, &edx);

                c->x86_capability[CPUID_1_ECX] = ecx;
                c->x86_capability[CPUID_1_EDX] = edx;
        }

        /* Thermal and Power Management Leaf: level 0x00000006 (eax) */
        if (c->cpuid_level >= 0x00000006)
                c->x86_capability[CPUID_6_EAX] = cpuid_eax(0x00000006);

        /* Additional Intel-defined flags: level 0x00000007 */
        if (c->cpuid_level >= 0x00000007) {
                cpuid_count(0x00000007, 0, &eax, &ebx, &ecx, &edx);
                c->x86_capability[CPUID_7_0_EBX] = ebx;
                c->x86_capability[CPUID_7_ECX] = ecx;
                c->x86_capability[CPUID_7_EDX] = edx;

                /* Check valid sub-leaf index before accessing it */
                if (eax >= 1) {
                            cpuid_count(0x00000007, 1, &eax, &ebx, &ecx, &edx);
                        c->x86_capability[CPUID_7_1_EAX] = eax;
                }
        }

        /* Extended state features: level 0x0000000d */
        if (c->cpuid_level >= 0x0000000d) {
                cpuid_count(0x0000000d, 1, &eax, &ebx, &ecx, &edx);

                c->x86_capability[CPUID_D_1_EAX] = eax;
        }

        /* AMD-defined flags: level 0x80000001 */
        eax = cpuid_eax(0x80000000);
        c->extended_cpuid_level = eax;

        if ((eax & 0xffff0000) == 0x80000000) {
                if (eax >= 0x80000001) {
                        cpuid(0x80000001, &eax, &ebx, &ecx, &edx);

                        c->x86_capability[CPUID_8000_0001_ECX] = ecx;
                        c->x86_capability[CPUID_8000_0001_EDX] = edx;
                }
        }


        if (c->extended_cpuid_level >= 0x80000007) {
                cpuid(0x80000007, &eax, &ebx, &ecx, &edx);

                c->x86_capability[CPUID_8000_0007_EBX] = ebx;
                c->x86_power = edx;
        }

        if (c->extended_cpuid_level >= 0x80000008) {
                cpuid(0x80000008, &eax, &ebx, &ecx, &edx);
                c->x86_capability[CPUID_8000_0008_EBX] = ebx;
        }

        if (c->extended_cpuid_level >= 0x8000000a)
                c->x86_capability[CPUID_8000_000A_EDX] = cpuid_edx(0x8000000a);

        init_scattered_cpuid_features(c);
        init_speculation_control(c);
        init_cqm(c);

        /*
         * Clear/Set all flags overridden by options, after probe.
         * This needs to happen each time we re-probe, which may happen
         * several times during CPU initialization.
         */
        apply_forced_caps(c);
}
void get_cpu_address_sizes(struct cpuinfo_x86 *c)
{
    u32 eax, ebx, ecx, edx;

    if (c->extended_cpuid_level >= 0x80000008) {
        cpuid(0x80000008, &eax, &ebx, &ecx, &edx);

        c->x86_virt_bits = (eax >> 8) & 0xff;
        c->x86_phys_bits = eax & 0xff;
    }
    c->x86_cache_bits = c->x86_phys_bits;
}



static void __init early_identify_cpu(struct cpuinfo_x86 *c)
{
    c->x86_clflush_size = 64;
    c->x86_phys_bits = 36;
    c->x86_virt_bits = 48;
    c->x86_cache_alignment = c->x86_clflush_size;

    memset(&c->x86_capability, 0, sizeof(c->x86_capability));
    c->extended_cpuid_level = 0;
    cpu_detect(c);
    get_cpu_cap(c);
    get_cpu_address_sizes(c);
    printk("x86_virt_bits:%d,x86_phys_bits:%d\n",c->x86_virt_bits, c->x86_phys_bits);
}


void __init init_cpuid()
{
    u32 eax = 1;
    u32 ecx = 0;
    u32 ebx, edx;
    early_identify_cpu(&boot_cpu_data);
    dump_mem(&cpu_caps_set,sizeof(cpu_caps_set));
    memset(&cpu_features,0, sizeof(cpu_features));
    native_cpuid(&eax, &ebx, &ecx, &edx);
    printk("cpu features:\n");
    if (test_bit(9, (ulong *) & edx)) {
        //xapic
    }
    if (test_bit(0, (ulong *) & ecx)) {
        cpu_features.sse3 = true;
        printk(" sse3");
    }
    if (test_bit(9, (ulong *) & ecx)) {
        cpu_features.ssse3 = true;
        printk(" ssse3");

    }
    if (test_bit(13, (ulong *) & ecx)) {
        cpu_features.cmpxchg16b = true;
        printk(" cmpxchg16b");

    }
    if (test_bit(19, (ulong *) & ecx)) {
        cpu_features.sse4_1 = true;
        printk(" sse4_1");

    }
    if (test_bit(20, (ulong *) & ecx)) {
        cpu_features.sse4_2 = true;
        printk(" sse4_2");

    }
 if (test_bit(21, (ulong *) & ecx)) {
        cpu_features.x2apic = true;
        printk(" x2apic");
    }

    if (test_bit(24, (ulong *) & ecx)) {
        cpu_features.tsc_deadline = true;
        printk(" tsc_deadline");
    }

    if (test_bit(26, (ulong *) & ecx)) {
        cpu_features.xsave = true;
        printk(" xsave");
    }

    if (test_bit(27, (ulong *) & ecx)) {
        cpu_features.osxsave = true;

        printk(" osxsave");
    }
  if (test_bit(28, (ulong *) & ecx)) {
        cpu_features.avx = true;
        printk(" avx");
    }

    if (test_bit(30, (ulong *) & ecx)) {
        cpu_features.rdrand = true;

        printk(" rdrand");
    }
    if (test_bit(19, (ulong *) & edx)) {
        cpu_features.clflush = true;

        printk(" clflush");
    }
    eax = 7;
    native_cpuid(&eax, &ebx, &ecx, &edx);
    ulong cr4 = read_cr4() |  cr4_de | cr4_pse | cr4_pae | cr4_pge | cr4_osfxsr
            | cr4_osxmmexcpt;

    if (test_bit(0, (ulong *) & ebx)) {
        cpu_features.fsgsbase = true;
        cr4 |= cr4_fsgsbase;
        printk(" fsgsbase");
    }
    write_cr4(cr4);
    if (cpu_features.xsave) {
        ulong bits = xcr0_x87 | xcr0_sse;
        if (cpu_features.avx) {
            bits |= xcr0_avx;
        }
    //    write_xcr(xcr0, bits);
    }

    if (test_bit(9, (ulong *) & ebx)) {
        cpu_features.repmovsb = true;

        printk(" repmovsb");
    }
    printk("\n");

}

void init_cpuid_ap()
{
    if (cpu_features.fsgsbase) {
        write_cr4(read_cr4() | cr4_fsgsbase);
    }
}
void __show_regs(struct trapframe * tf)
{
    unsigned long cr0 = 0L, cr2 = 0L, cr3 = 0L, cr4 = 0L, fs, gs, shadowgs;
    unsigned long d0, d1, d2, d3, d6, d7;
    unsigned int fsindex, gsindex;
    if (tf->trapno) {
        printk("APICID:%d Exception EIP:%04x:%016lx,RBP:%016lx,Trapno:%x\n",\
           cpuid_apic_id(),tf->cs,tf->rip, tf->rbp, tf->trapno);
        printk("EFLAGS:%016lx,RSP:%016lx,err:%lx,\n",tf->eflags,tf->rsp,tf->err);

    } else {
         printk("EIP:%04x:%pS,RBP:%016lx\n", tf->cs,tf->rip, tf->rbp);
         printk("EFLAGS:%016lx,RSP:%016lx\n",tf->eflags,tf->rsp);

    }
    printk("RAX: %016lx RBX: %016lx RCX: %016lx\n",tf->rax,tf->rbx,tf->rcx);
    printk("RDX: %016lx RSI: %016lx RDI: %016lx\n",tf->rdx,tf->rsi,tf->rdi);
    printk("RBP: %016lx R08: %016lx R09: %016lx\n",tf->rbp,tf->r8,tf->r9);
    printk("R10: %016lx R11: %016lx R12: %016lx\n",tf->r10,tf->r11,tf->r12);
    printk("R13: %016lx R14: %016lx R15: %016lx\n",tf->r13,tf->r14,tf->r15);
    fs = rdmsr(MSR_FS_BASE);
    gs = rdmsr(MSR_GS_BASE);
    shadowgs = rdmsr(MSR_KERNEL_GS_BASE);

    cr0 = read_cr0();
    cr2 = read_cr2();
    cr3 = read_cr3();
    cr4 = read_cr4();
    asm("movl %%fs,%0" : "=r" (fsindex));
    asm("movl %%gs,%0" : "=r" (gsindex));

    printk("FS:  %016lx(%04x) GS:%016lx(%04x) knlGS:%016lx\n",
              fs, fsindex, gs, gsindex, shadowgs);
    printk("CS:  %04lx DS: %04x ES: %04x CR0: %016lx\n", tf->cs, tf->ds,
                     read_es(), cr0);
    printk("CR2: %016lx CR3: %016lx CR4: %016lx\n", cr2, cr3,
                        cr4);

    get_debugreg(d0, 0);
    get_debugreg(d1, 1);
    get_debugreg(d2, 2);
    get_debugreg(d3, 3);
    get_debugreg(d6, 6);
    get_debugreg(d7, 7);

      /* Only print out debug registers if they are in their non-default state. */
    if (!((d0 == 0) && (d1 == 0) && (d2 == 0) && (d3 == 0) &&
        (d6 == DR6_RESERVED) && (d7 == 0x400))) {
            printk("DR0: %016lx DR1: %016lx DR2: %016lx\n",
                   d0, d1, d2);
            printk("DR3: %016lx DR6: %016lx DR7: %016lx\n",
                   d3, d6, d7);
    }

}
static spinlock_t spin_reg = 0;

void cprint_regs(struct trapframe *tf)
{
    unsigned long flag = local_irq_save();
    spin_lock(&spin_reg);
    __show_regs(tf);
    dump_mem64((char *)tf->rsp,128);
    ulong *p = (ulong *)tf->rbp;
    printk("call stack:\n");
    while((ulong)p>KERNEL_BASE) {
        printk(" %016lx ",*(p+1));
        p = (ulong *)*p;
    }
    printk("\n");
    spin_unlock(&spin_reg);
    local_irq_restore(flag);

}

