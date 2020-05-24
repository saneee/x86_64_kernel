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
    set_timeout(1,test);
    while(msec_count%100); 
    printk("test timeout: %lx,cpu:%d,thread:%s\n",nowmsec,smp_processor_id(),p->name);

}
static int test_init(bool isbp)
{
    set_timeout(11000,test);
    return 0;

}
late_initcall(test_init);

