#ifndef _YAOS_TASKLET_H
#define _YAOS_TASKLET_H
#include <yaos/atomic.h>
#include <yaos/percpu.h>
#define TASKLET_STACK_SIZE 0x2000
struct softirq_action {
    void (*action) (struct softirq_action *);
};
enum {
    HI_SOFTIRQ = 0,
    TIMER_SOFTIRQ,
    NET_TX_SOFTIRQ,
    NET_RX_SOFTIRQ,
    BLOCK_SOFTIRQ,
    BLOCK_IOPOLL_SOFTIRQ,
    TASKLET_SOFTIRQ,
    SCHED_SOFTIRQ,
    HRTIMER_SOFTIRQ,          
    RCU_SOFTIRQ,                /* Preferable RCU should always be the last softirq */

    NR_SOFTIRQS
};

#include <yaos/barrier.h>
struct tasklet_struct {
    struct tasklet_struct *next;
    unsigned long state;
    atomic_t count;
    void (*func) (unsigned long);
    unsigned long data;
};

#define DECLARE_TASKLET(name, func, data) \
struct tasklet_struct name = { NULL, 0, ATOMIC_INIT(0), func, data }

#define DECLARE_TASKLET_DISABLED(name, func, data) \
struct tasklet_struct name = { NULL, 0, ATOMIC_INIT(1), func, data }

enum {
    TASKLET_STATE_SCHED,        /* Tasklet is scheduled for execution */
    TASKLET_STATE_RUN           /* Tasklet is running (SMP only) */
};
static inline int tasklet_trylock(struct tasklet_struct *t)
{
    return !test_and_set_bit(TASKLET_STATE_RUN, &(t)->state);
}

static inline void tasklet_unlock(struct tasklet_struct *t)
{
    smp_mb__before_atomic();
    clear_bit(TASKLET_STATE_RUN, &(t)->state);
}

static inline void tasklet_unlock_wait(struct tasklet_struct *t)
{
    while (test_bit(TASKLET_STATE_RUN, &(t)->state)) {
        barrier();
    }
}

extern void __tasklet_schedule(struct tasklet_struct *t);

static inline void tasklet_schedule(struct tasklet_struct *t)
{
    if (!test_and_set_bit(TASKLET_STATE_SCHED, &t->state))
        __tasklet_schedule(t);
}

extern void __tasklet_hi_schedule(struct tasklet_struct *t);

static inline void tasklet_hi_schedule(struct tasklet_struct *t)
{
    if (!test_and_set_bit(TASKLET_STATE_SCHED, &t->state))
        __tasklet_hi_schedule(t);
}

extern void __tasklet_hi_schedule_first(struct tasklet_struct *t);

static inline void tasklet_hi_schedule_first(struct tasklet_struct *t)
{
    if (!test_and_set_bit(TASKLET_STATE_SCHED, &t->state))
        __tasklet_hi_schedule_first(t);
}

static inline void tasklet_disable_nosync(struct tasklet_struct *t)
{
    atomic_inc(&t->count);
    smp_mb__after_atomic();
}

static inline void tasklet_disable(struct tasklet_struct *t)
{
    tasklet_disable_nosync(t);
    tasklet_unlock_wait(t);
    smp_mb();
}

static inline void tasklet_enable(struct tasklet_struct *t)
{
    smp_mb__before_atomic();
    atomic_dec(&t->count);
}

DECLARE_PER_CPU(int, tasklet_count);
static inline bool in_tasklet()
{
    return this_cpu_read(tasklet_count) >= 0;
}

static inline void disable_tasklet()
{
    __this_cpu_inc(tasklet_count);
}

static inline void enable_tasklet()
{
    __this_cpu_dec(tasklet_count);
}

void open_softirq(int nr, void (*action) (struct softirq_action *));
void raise_softirq(unsigned int nr);
#endif
