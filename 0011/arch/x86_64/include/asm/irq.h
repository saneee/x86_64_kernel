#ifndef _ASM_X86_IRQ_H
#define _ASM_X86_IRQ_H
/*
 *	(C) 1992, 1993 Linus Torvalds, (C) 1997 Ingo Molnar
 *
 *	IRQ/IPI changes taken from work by Thomas Radke
 *	<tomsoft@informatik.tu-chemnitz.de>
 */

#include <asm/apicdef.h>
#include <asm/irq_vectors.h>
#include <asm/cpu.h>
#include <asm/cpu_flags.h>
#include <yaos/cache.h>
#include <yaos/percpu.h>

#define __ARCH_HAS_DO_SOFTIRQ
typedef void (*irq_handler_t) (int irq);
extern irq_handler_t irq_vectors[256];
static inline irq_handler_t register_irq(int irq, irq_handler_t pnew)
{
    int n = irq;
    irq_handler_t old = irq_vectors[n];

    irq_vectors[n] = pnew;
    return old;
}

static inline int arch_irqs_disabled_flags(unsigned long flags)
{
    return !(flags & X86_EFLAGS_IF);
}

static inline int arch_irqs_disabled(void)
{
    unsigned long flags = arch_local_save_flags();

    return arch_irqs_disabled_flags(flags);
}

typedef struct {
    unsigned int __softirq_pending;
    unsigned int __nmi_count;   /* arch dependent */
    unsigned int apic_timer_irqs;	/* arch dependent */
    unsigned int irq_spurious_count;
    unsigned int icr_read_retry_count;
    unsigned int x86_platform_ipis;	/* arch dependent */
    unsigned int apic_perf_irqs;
    unsigned int apic_irq_work_irqs;
    unsigned int irq_resched_count;
    unsigned int irq_call_count;
    /*
     * irq_tlb_count is double-counted in irq_call_count, so it must be
     * subtracted from irq_call_count when displaying irq_call_count
     */
    unsigned int irq_tlb_count;
} ____cacheline_aligned irq_cpustat_t;

//DECLARE_PER_CPU_SHARED_ALIGNED(irq_cpustat_t, irq_stat);

#define __ARCH_IRQ_STAT

#define inc_irq_stat(member)    this_cpu_inc(irq_stat.member)

#define local_softirq_pending() this_cpu_read(irq_stat.__softirq_pending)

#define __ARCH_SET_SOFTIRQ_PENDING

#define set_softirq_pending(x)  \
                this_cpu_write(irq_stat.__softirq_pending, (x))
#define or_softirq_pending(x)   this_cpu_or(irq_stat.__softirq_pending, (x))

#endif /* _ASM_X86_IRQ_H */
