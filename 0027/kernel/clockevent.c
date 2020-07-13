#include <types.h>
#include <yaos/list.h>
#include <yaos/smp.h>
static LIST_HEAD(clockevent_devices);
static DEFINE_RAW_SPINLOCK(clockevents_lock);
void clockevents_register_device(struct clock_event_device *dev)
{
    unsigned long flags;

    /* Initialize state to DETACHED */
    clockevent_set_state(dev, CLOCK_EVT_STATE_DETACHED);
    raw_spin_lock_irqsave(&clockevents_lock, flags);

    list_add(&dev->list, &clockevent_devices);

    raw_spin_unlock_irqrestore(&clockevents_lock, flags);
}
void clockevents_config_and_register(struct clock_event_device *dev,
                                     u32 freq, unsigned long min_delta,
                                     unsigned long max_delta)
{
    dev->min_delta_ticks = min_delta;
    dev->max_delta_ticks = max_delta;
    clockevents_register_device(dev);
}

