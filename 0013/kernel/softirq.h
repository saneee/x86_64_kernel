#ifndef __KERNEL_SOFTIRQ_H
#define __KERNEL_SOFTIRQ_H
#include <yaos/tasklet.h>
static inline void __raise_softirq_irqoff(int nr)
{
    or_softirq_pending(1UL << nr);

}

static inline void raise_softirq_irqoff(unsigned int nr)
{
    __raise_softirq_irqoff(nr);
    extern void wakeup_taskletd();
    /*
     * If we're in an interrupt or softirq, we're done
     * (this also catches softirq-disabled code). We will
     * actually run the softirq once we return from
     * the irq or softirq.
     *
     * Otherwise we wake up ksoftirqd to make sure we
     * schedule the softirq soon.
     */
    if (!in_tasklet())
        wakeup_taskletd();
}
#endif
