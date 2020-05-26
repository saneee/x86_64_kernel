#include <asm/pm64.h>
#include <asm/apic.h>
#include <asm/cpu.h>
#include <yaos/printk.h>
#include <types.h>
#include <yaos/init.h>
#include <asm/hpet.h>
#include <yaos/assert.h>
#include <yaos/percpu.h>
#include <asm/apic.h>
#include <yaos/irq.h>
#include <yaos/clockevent.h>
#define APIC_DIVISOR 16
#define FSEC_PER_NSEC 1000000UL
#define FSEC_PER_MSEC 1000000000000UL
static ulong start_tsc;
static ulong start_uptime;
static ulong clk_fsecs;
static ulong tsc_fsecs;
static ulong clk_khz;
static ulong tsc_khz;
struct clock_event_device;
unsigned long lapic_timer_period = 0;
static int lapic_timer_shutdown(struct clock_event_device *evt)
{
    unsigned int v;

    v = lapic_read(APIC_LVTT);
    v |= (APIC_LVT_MASKED | LOCAL_TIMER_VECTOR);
    lapic_write(APIC_LVTT, v);
    lapic_write(APIC_TMICT, 0);
    return 0;
}
static void __setup_APIC_LVTT(unsigned int clocks, int oneshot, int irqen)
{
    unsigned int lvtt_value, tmp_value;

    lvtt_value = LOCAL_TIMER_VECTOR;
    if (!oneshot)
        lvtt_value |= APIC_LVT_TIMER_PERIODIC;
    else if (boot_cpu_has(X86_FEATURE_TSC_DEADLINE_TIMER))
        lvtt_value |= APIC_LVT_TIMER_TSCDEADLINE;

    if (!lapic_is_integrated())
       lvtt_value |= SET_APIC_TIMER_BASE(APIC_TIMER_BASE_DIV);

    if (!irqen)
       lvtt_value |= APIC_LVT_MASKED;

    lapic_write(APIC_LVTT, lvtt_value);

    if (lvtt_value & APIC_LVT_TIMER_TSCDEADLINE) {
       /*
        * See Intel SDM: TSC-Deadline Mode chapter. In xAPIC mode,
        * writing to the APIC LVTT and TSC_DEADLINE MSR isn't serialized.
        * According to Intel, MFENCE can do the serialization here.
        */
        asm volatile("mfence" : : : "memory");
           return;
    }

    /*
     * Divide PICLK by 16
     */
    tmp_value = lapic_read(APIC_TDCR);
    lapic_write(APIC_TDCR,
         (tmp_value & ~(APIC_TDR_DIV_1 | APIC_TDR_DIV_TMBASE)) |
            APIC_TDR_DIV_16);

    if (!oneshot)
        lapic_write(APIC_TMICT, clocks / APIC_DIVISOR);
}

static inline int
lapic_timer_set_periodic_oneshot(struct clock_event_device *evt, bool oneshot)
{
     __setup_APIC_LVTT(lapic_timer_period, oneshot, 1);
    return 0;
}

static int lapic_timer_set_periodic(struct clock_event_device *evt)
{
    return lapic_timer_set_periodic_oneshot(evt, false);
}

static int lapic_timer_set_oneshot(struct clock_event_device *evt)
{
    return lapic_timer_set_periodic_oneshot(evt, true);
}
static int lapic_next_event(unsigned long delta_ns,
                            struct clock_event_device *evt)
{
    lapic_write(APIC_TMICT, (delta_ns * (FSEC_PER_NSEC / APIC_DIVISOR )/ clk_fsecs) || 1);
    return 0;
}
static int lapic_next_deadline(unsigned long delta_ns,
                               struct clock_event_device *evt)
{
    u64 tsc;

    tsc = rdtsc();
    wrmsr(MSR_IA32_TSC_DEADLINE, tsc + (((u64) delta_ns * FSEC_PER_NSEC / tsc_fsecs) ));
    return 0;
}

static struct clock_event_device lapic_clockevent = {
        .name                           = "lapic",
        .features                       = CLOCK_EVT_FEAT_PERIODIC |
                                          CLOCK_EVT_FEAT_ONESHOT,
        .set_state_shutdown             = lapic_timer_shutdown,
        .set_state_periodic             = lapic_timer_set_periodic,
        .set_state_oneshot              = lapic_timer_set_oneshot,
        .set_state_oneshot_stopped      = lapic_timer_shutdown,
        .set_next_event                 = lapic_next_event,
        .irq                            = -1,
};
DECLARE_PER_CPU(struct clock_event_device, the_cev);
static __init int calibrate_APIC_clock()
{
    unsigned long flags;

    flags = local_irq_save();
    ulong now_uptime = hpet_uptime();
    ulong now_tsc = rdtsc();
    ulong remain = lapic_read(APIC_TMCCT);
    local_irq_restore(flags);

    ulong start = start_uptime;
    ulong delta = now_uptime - start;
    ASSERT(delta<0x400000000);//纳秒单位大概16秒,超过可能apic_timer32位寄存器溢出
    ulong clock = delta*1000/(APIC_DIVISOR*(0xffffffff-remain));
    clock+=5;
    clock/=10;
    clock*=10000UL;
    clk_fsecs= clock;
    clk_khz = FSEC_PER_MSEC / clk_fsecs;
    lapic_timer_period = FSEC_PER_MSEC / APIC_DIVISOR / clock; //1ms clocks 
    printk("now_uptime:%ld,delta:%ld,start:%ld,reamin:%lx,clock:%ld,period:%d,khz:%ld,cpu:%d\n",now_uptime,now_uptime-start,start,remain,clock,lapic_timer_period,clk_khz,cpuid_apic_id());
    clock = 1000*delta/(now_tsc - start_tsc);
    clock*=1000UL;
    tsc_fsecs = clock;
    tsc_khz = FSEC_PER_MSEC / tsc_fsecs;
    printk("tsc:%lx,%lx,%lx,%lx,clock:%ld,khz:%ld\n",now_tsc,start_tsc,now_tsc-start_tsc,delta,clock,tsc_khz);
    return 0;
}
static int setup_APIC_timer(void)
{
    struct clock_event_device *levt = this_cpu_ptr(&the_cev);

    memcpy(levt, &lapic_clockevent, sizeof(*levt));

    if (boot_cpu_has(X86_FEATURE_TSC_DEADLINE_TIMER)) {
        levt->name = "lapic-deadline";
        levt->features &= ~(CLOCK_EVT_FEAT_PERIODIC); 
        levt->set_next_event = lapic_next_deadline;
        levt->min_delta_ns = tsc_fsecs/FSEC_PER_NSEC;
        levt->max_delta_ns = (0x7fffffff*tsc_fsecs/FSEC_PER_NSEC)<<32;
        levt->khz = tsc_khz;
        lapic_timer_period = FSEC_PER_MSEC / tsc_fsecs; //1ms clocks

    } else {
        levt->min_delta_ns = clk_fsecs*APIC_DIVISOR/FSEC_PER_NSEC;
        levt->max_delta_ns = 0x7fffffff*clk_fsecs/FSEC_PER_NSEC*APIC_DIVISOR;
        levt->khz = clk_khz;
        lapic_timer_period = FSEC_PER_MSEC / APIC_DIVISOR / clk_fsecs; //1ms clocks

    }
    levt->set_state_oneshot(levt);
    printk("min:%ld,max:%ld\n",levt->min_delta_ns,levt->max_delta_ns);
    return 0;
}

static __init int init_apic_timer_call(bool isbp)
{
    if(isbp) {
        ASSERT(hpet_period);
        lapic_write(APIC_SPIV, APIC_SPIV_APIC_ENABLED | SPURIOUS_APIC_VECTOR);
        lapic_write(APIC_TDCR, APIC_TDR_DIV_16);
        lapic_write(APIC_LVTT, LOCAL_TIMER_VECTOR);
        lapic_write(APIC_TMICT, 0xFFFFFFFF);

        start_uptime = hpet_uptime();
        start_tsc = rdtsc();
        printk("cpuid_apic_id:%d,isbp:%d\n",cpuid_apic_id(),isbp);
    } 
    return 0;
    
}
static __init int init_apic_timer_second(bool isbp)
{
printk("boot_cpu_has(X86_FEATURE_TSC_DEADLINE_TIMER):%d\n",boot_cpu_has(X86_FEATURE_TSC_DEADLINE_TIMER));
   return (isbp && calibrate_APIC_clock()) || setup_APIC_timer(); 
}
core_initcall_sync(init_apic_timer_call);
postcore_initcall_sync(init_apic_timer_second);
