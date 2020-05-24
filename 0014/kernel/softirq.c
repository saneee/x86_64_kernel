#include <yaos/types.h>
#include <yaos/tasklet.h>
#include <yaos/barrier.h>
#include <yaos/percpu.h>
#include <yaos/irq.h>
#include <yaos/kernel.h>
#include <yaos/bug.h>
#include <yaos/init.h>
#include <yaos/sched.h>
#include <yaos/smp.h>
#include <yaos/time.h>
#include <asm/apic.h>
#include "softirq.h"
void __do_softirq(void);
extern void wakeup_taskletd(void);
extern __thread ulong msec_count;
extern int irq_count;
static struct softirq_action softirq_vec[NR_SOFTIRQS]
 __cacheline_aligned_in_smp;

const char *const softirq_to_name[NR_SOFTIRQS] = {
    "HI", "TIMER", "NET_TX", "NET_RX", "BLOCK", "BLOCK_IOPOLL",
    "TASKLET", "SCHED", "HRTIMER", "RCU"
};

static inline void invoke_softirq(void)
{
    __do_softirq();
}

void irq_enter(void)
{
}

void irq_exit(void)
{
    local_irq_disable();
    if (!in_tasklet() && local_softirq_pending())
        invoke_softirq();

}


void raise_softirq(unsigned int nr)
{
    unsigned long flags;

    flags = local_irq_save();
    raise_softirq_irqoff(nr);
    local_irq_restore(flags);

}

void open_softirq(int nr, void (*action) (struct softirq_action * p))
{
    softirq_vec[nr].action = action;

}

/* 2ms */
#define MAX_SOFTIRQ_TIME 2
#define MAX_SOFTIRQ_RESTART 10
void __do_softirq(void)
{
    unsigned long end = msec_count + MAX_SOFTIRQ_TIME;
    int max_restart = MAX_SOFTIRQ_RESTART;
    struct softirq_action *h;
    __u32 pending;
    int softirq_bit;

    /*
     * Mask out PF_MEMALLOC s current task context is borrowed for the
     * softirq. A softirq handled such as network RX might set PF_MEMALLOC
     * again if the socket is related to swap
     */
    pending = local_softirq_pending();
    disable_tasklet();
  restart:
    /* Reset the pending bitmask before enabling irqs */
    set_softirq_pending(0);

    local_irq_enable();

    h = softirq_vec;

    while ((softirq_bit = ffs(pending))) {

        h += softirq_bit - 1;

        h->action(h);
        h++;
        pending >>= softirq_bit;
    }

    local_irq_disable();
    pending = local_softirq_pending();
    if (pending) {
        if (msec_count < end && --max_restart)
            goto restart;
        enable_tasklet();
        wakeup_taskletd();
    } else enable_tasklet();
}

void do_softirq(void)
{
    __u32 pending;
    unsigned long flags;

    if (in_tasklet())
        return;

    flags = local_irq_save();

    pending = local_softirq_pending();

    if (pending)
        __do_softirq();

    local_irq_restore(flags);
}





