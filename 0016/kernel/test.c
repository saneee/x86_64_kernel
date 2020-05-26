#include <types.h>
#include <yaos/init.h>
#include <yaos/time.h>
#include <yaos/printk.h>
#include <yaos/smp.h>
#include <yaos/cpupm.h>
#include <yaos/tasklet.h>
#include <yaos/sched.h>
#include <asm/current.h>
extern __thread ulong msec_count;
static void test_nsec (u64 nownsec,void *param)
{
    pthread p = current;
    pthread powner = (pthread)param;

    set_timeout_nsec(10000000000,test_nsec,param);
    printk("test_nsec timeout uptime:%ld,cpu:%d,current_thread:%s,owner:%s\n",
       hrt_uptime()/1000000000,smp_processor_id(),p->name,powner->name);

}
static void test_sleep()
{
    
    while(1) {
        pthread p = current;
        sleep(10);
        printk("test_sleep uptime:%ld,cpu:%d,current_thread:%s\n",
            hrt_uptime()/1000000000,smp_processor_id(),p->name);
    }
}
int test_should_run(unsigned int cpu)
{
    return 1;
}
static int run_test_thread(unsigned long cpu)
{
    test_sleep();
    return 0;
}
DEFINE_PER_CPU(struct task_struct, test_task);


static struct smp_percpu_thread test_threads = {
    .task = &test_task,
    .thread_should_run = test_should_run,
    .thread_fn = run_test_thread,
    .thread_comm = "test",
    .lvl = THREAD_LVL_USER,
    .stack_size = 0x1000
};
static int test_init(bool isbp)
{
    set_timeout_nsec(10000000000,test_nsec,current);
    struct task_struct *tsk = this_cpu_ptr(&test_task);
    wake_up_thread(&tsk->mainthread);
    return 0;

}
static int test_init_early(bool isbp)
{
    if(isbp){
       struct task_struct *tsk = this_cpu_ptr(&test_task);
       memset(tsk, 0, sizeof(*tsk));
       smpboot_register_percpu_thread(&test_threads);
    }
    return 0;
}

early_initcall(test_init_early);

late_initcall(test_init);

