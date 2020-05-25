#include <types.h>
#include <yaos/init.h>
#include <yaos/time.h>
#include <yaos/printk.h>
#include <yaos/smp.h>
#include <yaos/cpupm.h>
#include <yaos/tasklet.h>
#include <yaos/sched.h>
extern __thread ulong msec_count;
static void test (u64 nowmsec) 
{
    cpu_p pcpu = get_current_cpu();
    struct thread_struct *p = pcpu->current_thread;
    set_timeout(10000,test);
    printk("test timeout: %lx,uptime:%ld,cpu:%d,thread:%s\n",nowmsec,hrt_uptime()/1000000000,smp_processor_id(),p->name);

}
static void test_nsec (u64 nownsec)
{
    cpu_p pcpu = get_current_cpu();
    struct thread_struct *p = pcpu->current_thread;
    set_timeout_nsec(10000000000,test_nsec);
    printk("test_nsec timeout uptime:%ld,cpu:%d,thread:%s\n",hrt_uptime()/1000000000,smp_processor_id(),p->name);

}

static int test_init(bool isbp)
{
    set_timeout(10000,test);
    set_timeout_nsec(10000000000,test_nsec);
    return 0;

}
late_initcall(test_init);

