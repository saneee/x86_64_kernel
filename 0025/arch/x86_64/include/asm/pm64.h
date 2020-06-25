/* zhuhanjun 2015/10/29 
per cpu struct
*/
#ifndef ARCH_X86_64_PM64_H
#define ARCH_X86_64_PM64_H 1
#define INTR_GATE_TYPE 14
//first 4K is hole
#define INIT_STACK_SIZE (4096*3)
#ifndef PAGE_4K_SIZE
#define PAGE_4K_SIZE 0x1000
#endif
#define EXCEPTION_STKSZ PAGE_4K_SIZE
#define IRQ_STACK_SIZE (PAGE_4K_SIZE*8)
#define PG_STACK_SIZE (PAGE_4K_SIZE*4)
#define DEBUG_STKSZ (PAGE_4K_SIZE*2)
#define DOUBLEFAULT_STACK 1
#define NMI_STACK 2
#define DEBUG_STACK 3
#define MCE_STACK 4
#define DEFAULT_EXCEPT_STACK 5
#define IRQ_STACK 6
#define PG_STACK 7
#define N_EXCEPTION_STACKS 7    /* hw limit: 7 */

#ifndef __ASSEMBLY__
#include <yaos/types.h>
#include <yaos/kheap.h>
#include <yaos/printk.h>
#include <asm/cpu.h>
#include <asm/percpu.h>
#include <yaos/cpupm.h>
#include <yaos/rett.h>
/*
 * Save the original ist values for checking stack pointers during debugging
 */
struct orig_ist {
    unsigned long ist[8];       //0 reserved
};

DECLARE_PER_CPU(struct orig_ist, orig_ist);

DECLARE_PER_CPU(char *, irq_stack_ptr);
DECLARE_PER_CPU(int, irq_count);
struct irq_stack {
    u32 stack[PAGE_4K_SIZE / sizeof(u32)];
}
__aligned(PAGE_4K_SIZE);

DECLARE_PER_CPU(struct irq_stack *, hardirq_stack);
DECLARE_PER_CPU(struct irq_stack *, softirq_stack);
struct init_stack {
    char stack[INIT_STACK_SIZE] __aligned(PAGE_4K_SIZE);
};
DECLARE_PER_CPU(struct init_stack, init_stack);

struct trapframe {
    u64 rax;
    u64 rbx;
    u64 rcx;
    u64 rdx;
    u64 rbp;
    u64 rsi;
    u64 rdi;
    u64 r8;
    u64 r9;
    u64 r10;
    u64 r11;
    u64 r12;
    u64 r13;
    u64 r14;
    u64 r15;

    u64 trapno;
    u64 err;

    u64 rip;                    // rip
    u64 cs;
    u64 eflags;                 // rflags
    u64 rsp;                    // rsp
    u64 ds;                     // ss
};
extern ret_t switch_to(ulong stack, ulong * poldstack, ulong arg);
extern ret_t switch_first(struct thread_struct *, void *, ulong, ulong *,
                          ulong);

static inline bool is_bp_cpu(cpu_p cpu)
{
    return (cpu->arch_cpu.apic_id == 0);
}

#endif
#endif
