#include <yaos/assert.h>
#include <asm/cpu.h>
#include <asm/string.h>
#include <types.h>
#include <yaos/printk.h>
#include <asm/bitops.h>
#include <asm/pm64.h>
#include <asm/msrdef.h>
#include <yaos/spinlock.h>
#include <yaos/irq.h>
struct cpu_features_type cpu_features;

void init_cpuid()
{
    u32 eax = 1;
    u32 ecx = 0;
    u32 ebx, edx;

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
    if (test_bit(0, (ulong *) & ebx)) {
        cpu_features.fsgsbase = true;
        write_cr4(read_cr4() | cr4_fsgsbase);
        printk(" fsgsbase");
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
        printk("Exception EIP:%04x:%pS,RBP:%016lx,Trapno:%x\n", tf->cs,tf->rip, tf->rbp, tf->trapno);
        printk("EFLAGS:%016lx,RSP:%016lx,err:%lx\n",tf->eflags,tf->rsp,tf->err);

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
    spin_unlock(&spin_reg);
    local_irq_restore(flag);

}

