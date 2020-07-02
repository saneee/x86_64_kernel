#include <yaos/cpupm.h>
#include <yaos/list.h>

#include <yaos/smp.h>
#include <yaos/percpu.h>
#include <yaos/assert.h>
#include <yaos/init.h>
#include <yaos/sched.h>
#include <yaos/tasklet.h>
#include <asm/pgtable.h>
#include <yaos/kheap.h>
#include <errno.h>
int nr_cpu = 0;
extern int irq_count;
char *new_percpu()
{
    char *p = kalloc((__per_cpu_end - __per_cpu_start));

    memcpy(p, (char *)__per_cpu_load, (__per_cpu_end - __per_cpu_start));
    cpu_p pcpu = (cpu_p)p;
    printk("irq_count_new:%d\n",PER_CPU_PTR_VAR(&irq_count,pcpu));
    return p;
}

static LIST_HEAD(percpu_threads);

void smp_init()
{

    arch_start_aps();

}

static int smp_thread_main(struct smp_percpu_thread *ht)
{
    printk("smp_thread_main:%lx\n", ht);
    suspend_thread();

    while (true) {
        if (ht->thread_should_run(ht->cpu)) {
            ht->thread_fn(ht->cpu);
        }
        suspend_thread();
    }
    return 0;
}

int smp_create_thread(struct smp_percpu_thread *ht, int cpu)
{
    printk("ht:%lx,%x\n", ht, cpu);
    ht->cpu = cpu;
    struct task_struct *task = per_cpu_ptr(ht->task, cpu);
printk("task:%lx\n",task);
    task->mainthread.stack_addr = 0;
    task->mainthread.stack_size = ht->stack_size;
    task->mainthread.task = task;
    task->mainthread.main = (thread_func) smp_thread_main;
    task->mainthread.flag = THREAD_PERCPU;
    task->mainthread.name = ht->thread_comm;
    /*
    if (task->mainthread.stack_size < PAGE_SIZE_LARGE) {
        unsigned long addr =
            (ulong) alloc_kheap_4k(task->mainthread.stack_size);
        if (unlikely(!addr)) {
            return ENOMEM;
        }
        task->mainthread.stack_addr = addr + task->mainthread.stack_size;
    }
    */
    INIT_LIST_HEAD(&task->threads);

    int ret = create_thread_oncpu(&task->mainthread, cpu);

    if (ret)
        return ret;
    task->mainthread.real_parent = current;
    task->mainthread.lvl = ht->lvl;
    printk("cpu:%d,flag:%lx,state:%lx\n", cpu, task->mainthread.flag,
           task->mainthread.state);
    resume_thread(&task->mainthread, (ulong) ht);
    printk("name:%s,flag:%lx\n", task->mainthread.name, task->mainthread.flag);
    return 0;

}

static int init_percpu_thread(bool isbp)
{
    int n = smp_processor_id();
    struct smp_percpu_thread *p;

    list_for_each_entry(p, &percpu_threads, list) {
        int ret = smp_create_thread(p, n);

        if (ret)
            panic("smp_create_thread error:%d\n", ret);
    }
    printk("init_percpu_thread:%d done\n",isbp);
    return 0;
}

static bool ap_started = false;
void smp_ap_init()
{

    ap_started = true;
    printk("smp_processor_id:%d\n", smp_processor_id());
}

int smpboot_register_percpu_thread(struct smp_percpu_thread *plug_thread)
{
    int ret = 0;

    ASSERT(!ap_started);
    list_add(&plug_thread->list, &percpu_threads);
    return ret;
}

late_initcall(init_percpu_thread);
