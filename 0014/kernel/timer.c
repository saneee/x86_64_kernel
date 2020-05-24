#include <yaos/types.h>
#include <yaos/init.h>
#include <yaos/percpu.h>
#include <errno.h>
#include <yaoscall/malloc.h>
#include <yaos/sched.h>
#include <yaos/irq.h>
#include <yaos/assert.h>
#include <yaos/tasklet.h>
#define NR_TIMER_BUF 16
extern u64 msec_count;
typedef void (*timer_callback) (u64 nowmsec);
struct timeout_callback;
struct timeout_callback {
    struct timeout_callback *pnext;
    u64 deadline;
    void (*callback) (u64 nowmsec);
};
struct timer_manager {
    struct timeout_callback *pfree;
    struct timeout_callback *ptimer;
    struct timeout_callback timerbuf[NR_TIMER_BUF];
};
__percpu_data static struct timer_manager mtime;
int set_timeout(u64 msec, void (*pfunc) (u64 nowmsec))
{
    __thread struct timer_manager *ptime = PERCPU_PTR(&mtime);
    struct timeout_callback *p = ptime->pfree;

    if (!p) {
        p = yaos_malloc(sizeof(*p));
        if (!p)
            return ENOMEM;
    }
    else {
        ptime->pfree = p->pnext;
    }
    u64 thedeadline = msec + msec_count;

    p->deadline = thedeadline;
    p->callback = pfunc;
    unsigned long flag = local_irq_save();

    barrier();

    if (!ptime->ptimer || thedeadline < ptime->ptimer->deadline) {
        p->pnext = ptime->ptimer;
        ptime->ptimer = p;
    }
    else {
        struct timeout_callback *ptr = ptime->ptimer;

        while (ptr->pnext && ptr->pnext->deadline < thedeadline) {
            ptr = ptr->pnext;
        }
        p->pnext = ptr->pnext;
        ptr->pnext = p;

    }
    barrier();
    local_irq_restore(flag);
    return 0;
}

void run_local_timers(void)
{
    raise_softirq(TIMER_SOFTIRQ);
}

void run_timer_softirq(struct softirq_action *paction)
{
    struct timer_manager *ptime = this_cpu_ptr(&mtime);
    if (!ptime->ptimer || msec_count < ptime->ptimer->deadline) {
        return;
    }

    struct timeout_callback *p = ptime->ptimer;
    struct timeout_callback *ptr = p;
    timer_callback funcarr[10];
    int callnr = 10;

    ASSERT(p);
    funcarr[--callnr] = ptr->callback;
    while (ptr->pnext && msec_count >= ptr->pnext->deadline && callnr > 0) {
        ptr = ptr->pnext;
        funcarr[--callnr] = ptr->callback;

    }
    ptime->ptimer = ptr->pnext;
    ptr->pnext = ptime->pfree;
    ptime->pfree = p;

    barrier();
    unsigned flag = local_save_flags();

    local_irq_enable();
    for (int i = 9; i >= callnr; i--) {
        (funcarr[i]) (msec_count);
    }
    local_irq_restore(flag);
    if (callnr == 0)
        run_timer_softirq(paction);
}

time64_t mktime64(const unsigned int year0, const unsigned int mon0,
                  const unsigned int day, const unsigned int hour,
                  const unsigned int min, const unsigned int sec)
{
    unsigned int mon = mon0, year = year0;

    /* 1..12 -> 11,12,1..10 */
    if (0 >= (int)(mon -= 2)) {
        mon += 12;              /* Puts Feb last since it has leap day */
        year -= 1;
    }

    return ((((time64_t)
              (year / 4 - year / 100 + year / 400 + 367 * mon / 12 + day) + year * 365 - 719499) * 24 + hour	/* now have hours */
            ) * 60 + min        /* now have minutes */
        ) * 60 + sec;           /* finally seconds */
}

__init int static init_timer(bool isbp)
{
    struct timer_manager *ptime = PERCPU_PTR(&mtime);

    for (int i = 0; i < NR_TIMER_BUF - 1; i++) {
        ptime->timerbuf[i].pnext = &ptime->timerbuf[i + 1];
    }
    ptime->timerbuf[NR_TIMER_BUF - 1].pnext = NULL;

    ptime->pfree = &ptime->timerbuf[0];
    ptime->ptimer = NULL;
    if (isbp)
        open_softirq(TIMER_SOFTIRQ, run_timer_softirq);

    return 0;
}

early_initcall(init_timer);
