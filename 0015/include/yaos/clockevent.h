#ifndef _YAOS_CLOCKEVENT_H
#define _YAOS_CLOCKEVENT_H
#include <yaos/list.h>
# define CLOCK_EVT_FEAT_PERIODIC        0x000001
# define CLOCK_EVT_FEAT_ONESHOT         0x000002
enum clock_event_state {
        CLOCK_EVT_STATE_DETACHED,
        CLOCK_EVT_STATE_SHUTDOWN,
        CLOCK_EVT_STATE_PERIODIC,
        CLOCK_EVT_STATE_ONESHOT,
        CLOCK_EVT_STATE_ONESHOT_STOPPED,
};
struct clock_event_device {
    void                    (*event_handler)(struct clock_event_device *);
    int                     (*set_next_event)(unsigned long evt, struct clock_event_device *);
    int                     (*set_state_periodic)(struct clock_event_device *);
    int                     (*set_state_oneshot)(struct clock_event_device *);
    int                     (*set_state_oneshot_stopped)(struct clock_event_device *);
    int                     (*set_state_shutdown)(struct clock_event_device *);
    u64			    khz;
    u64                     max_delta_ns;
    u64                     min_delta_ns;
    u32                     mult;
    u32                     shift;
    enum clock_event_state  state;
    unsigned int            features;
    int			    irq;
    unsigned long           retries;
    const char              *name;
    struct list_head        list;

} ____cacheline_aligned;
#endif
