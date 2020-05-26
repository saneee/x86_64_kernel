#ifndef _YAOS_IRQ_H
#define _YAOS_IRQ_H
#include <asm/irq.h>
#include <asm/cpu.h>
#include <asm/percpu.h>
#ifdef __ARCH_HAS_DO_SOFTIRQ
void do_softirq_own_stack(void);
#else
static inline void do_softirq_own_stack(void)
{
    __do_softirq();
}
#endif

static inline unsigned long local_irq_save(void)
{
    return arch_local_irq_save();
}

static inline void local_irq_restore(unsigned long flag)
{
    arch_local_irq_restore(flag);
}

static inline void local_irq_enable(void)
{
    arch_local_irq_enable();
}

static inline void local_irq_disable(void)
{
    arch_local_irq_disable();
}

static inline unsigned long local_save_flags()
{
    return arch_local_save_flags();
}

static inline bool irqs_disabled()
{
    return arch_irqs_disabled();
}

extern void irq_enter();
extern void irq_exit();
#endif
