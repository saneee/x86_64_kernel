#include <yaos/types.h>
#include <yaos/time.h>
#include <asm/cpu.h>
#include <yaos/printk.h>
#include <string.h>
#include <asm/apic.h>
#include <asm/irq_vectors.h>
#include <yaos/init.h>
#include <asm/irq.h>
#include <yaos/assert.h>
#include <yaos/sched.h>
static void microdelay(int us)
{
}

#define CMOS_PORT    0x70
#define CMOS_RETURN  0x71

// Start additional processor running entry code at addr.
// See Appendix B of MultiProcessor Specification.

#define CMOS_STATA   0x0a
#define CMOS_STATB   0x0b
#define CMOS_UIP    (1 << 7)    // RTC update in progress

#define SECS    0x00
#define MINS    0x02
#define HOURS   0x04
#define DAY     0x07
#define MONTH   0x08
#define YEAR    0x09
#define MAX_TIMEOUT_CALLBACK 1000
static uint cmos_read(uint reg)
{
    outb(reg, CMOS_PORT);
    microdelay(200);

    return inb(CMOS_RETURN);
}

static void fill_rtcdate(struct rtcdate *r)
{
    r->second = cmos_read(SECS);
    r->minute = cmos_read(MINS);
    r->hour = cmos_read(HOURS);
    r->day = cmos_read(DAY);
    r->month = cmos_read(MONTH);
    r->year = cmos_read(YEAR);
}

// qemu seems to use 24-hour GWT and the values are BCD encoded
void cmostime(struct rtcdate *r)
{
    struct rtcdate t1, t2;
    int sb, bcd;

    sb = cmos_read(CMOS_STATB);

    bcd = (sb & (1 << 2)) == 0;

    // make sure CMOS doesn't modify time while we read it
    for (;;) {
        fill_rtcdate(&t1);
        if (cmos_read(CMOS_STATA) & CMOS_UIP)
            continue;
        fill_rtcdate(&t2);
        if (memcmp(&t1, &t2, sizeof(t1)) == 0)
            break;
    }

    // convert
    if (bcd) {
#define    CONV(x)     (t1.x = ((t1.x >> 4) * 10) + (t1.x & 0xf))
        CONV(second);
        CONV(minute);
        CONV(hour);
        CONV(day);
        CONV(month);
        CONV(year);
#undef     CONV
    }

    *r = t1;
    r->year += 2000;
}

static struct rtcdate mtime;
DEFINE_PER_CPU(unsigned long, the_ticks) __visible = 0;

u64 msec_count = 0;
u64 start_time_in_sec;
static void timer_irq_handler(int n)
{
    void run_local_timers(void);

    ack_lapic_irq();

    barrier();
    ++msec_count;
    barrier();
    run_local_timers();
    if (++msec_count%10000==0)printk("msec_count:%ld\n",msec_count);

}
static void hrt_timer_irq_handler(int n)
{
    void run_hrt_timers(void);

    ack_lapic_irq();
    run_hrt_timers();

}

static void local_timer_irq_handler(int n)
{
    void run_hrt_timers(void);

    ack_lapic_irq();
    run_hrt_timers();
    this_cpu_add_1(the_ticks,1);
    
}

static void timer_irq_first_handler(int n)
{
    ASSERT(msec_count == 0);
    ack_lapic_irq();
    cmostime(&mtime);
    printk("cmos:%d-%d-%d %d:%d:%d\n", mtime.year, mtime.month, mtime.day,
           mtime.hour, mtime.minute, mtime.second);

    register_irq(TIMER_VECTOR, timer_irq_handler);
    register_irq(LOCAL_TIMER_VECTOR, local_timer_irq_handler);

}

void __init init_time()
{
    cmostime(&mtime);
    printk("%d-%d-%d %d:%d:%d\n", mtime.year, mtime.month, mtime.day,
           mtime.hour, mtime.minute, mtime.second);
    start_time_in_sec =
        mktime64(mtime.year, mtime.month, mtime.day, mtime.hour, mtime.minute,
                 mtime.second);
    printk("start_time_in_sec:%ld\n", start_time_in_sec);
    register_irq(TIMER_VECTOR, timer_irq_first_handler);
    register_irq(HRT_TIMER_VECTOR, hrt_timer_irq_handler);

}

