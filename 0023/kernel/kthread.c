#include <yaos/types.h>
#include <yaos/spinlock.h>
#include <yaos/sched.h>
#include <yaos/list.h>
#include <yaos/percpu.h>
#include <errno.h>
#include <yaos/printk.h>
#include <yaos/vm.h>
#include <asm/cpu.h>
#include <asm/pgtable.h>
#include <yaos/cpupm.h>
#include <yaos/atomic.h>
#include <asm/pm64.h>
#include <yaos/init.h>
#include <yaos/assert.h>
#include <yaos/time.h>
#include <yaos/smp.h>
#include <yaos/irq.h>
#if 1
#define DEBUG_PRINT printk
#else
#define DEBUG_PRINT inline_DEBUG_PRINT

#endif
#define IDLE_STACK_SIZE 0x1000
#define KMAIN_STACK_SIZE 0x200000
extern void switch_idle(ulong stack);
extern ret_t switch_to(ulong stack, ulong * poldstack, ulong arg);
extern ret_t switch_first(struct thread_struct *, void *, ulong, ulong *, ulong);	//switch stack
extern ulong __max_phy_mem_addr;
u64 get_pte_with_addr(u64 addr);

ret_t yield_thread(ulong arg);
atomic_t idlenr = ATOMIC_INIT(0);

static DEFINE_SPINLOCK(kthead_create_lock);
static LIST_HEAD(kthread_create_list);
DEFINE_PER_CPU(struct task_struct, idle_task);
DEFINE_PER_CPU(struct thread_struct, kmain_thread);

static inline ret_t thread_switch_first(struct thread_struct *p,
	void *thread_main,
	struct thread_struct *old, ulong arg)
{
    __thread ulong stack_point;

    stack_point = p->stack_addr + p->stack_size - 8;
    DEBUG_PRINT("switch first to %s %lx,from:%s cpu:%d\n",
        p->name,stack_point,old->name,smp_processor_id());
    DEBUG_PRINT("new stack:%lx,old stack:%lx\n",stack_point,old->rsp);
    ASSERT(stack_point);
    ASSERT(stack_point < __max_phy_mem_addr || get_pte_with_addr(stack_point));
    return switch_first(p, thread_main, stack_point, &old->rsp, arg);
}

static inline ret_t thread_switch_to(struct thread_struct *p,
                                     struct thread_struct *old, ulong arg)
{
    if (!p->rsp){
 
        DEBUG_PRINT("%s to %s ,%lx,%lx,%lx,cpu:%d\n", 
           old->name,p->name, p, p->flag, p->state,smp_processor_id());
        DEBUG_PRINT("switch to new rsp:%lx,oldsp:%lx,&old->rsp:%lx,arg:%lx\n",
           p->rsp,old->rsp,&old->rsp,arg);
    }
    ASSERT(p->rsp);
    ASSERT(p->rsp > KERNEL_STACK_ZONE_START);
    return switch_to(p->rsp, &old->rsp, arg);
}

int create_thread_oncpu(struct thread_struct *pthread, int oncpu)
{
    DEBUG_PRINT("create_thread_oncpu:%016lx,stack:%lx,name:%s %d\n",
         pthread,pthread->stack_addr,pthread->name,oncpu);
    if (likely(!pthread->stack_addr)) {
        ulong stack_addr = (ulong) alloc_vm_stack(pthread->stack_size);

        if (unlikely(!stack_addr)) {
            DEBUG_PRINT("no memory alloc stack:%lx\n", pthread->stack_size);
            return ENOMEM;
        }
        pthread->stack_addr = stack_addr;

    }
    spin_lock(&kthead_create_lock);
    list_add(&pthread->threads, &kthread_create_list);
    spin_unlock(&kthead_create_lock);
    spin_lock(&pthread->task->tasklock);
    list_add(&pthread->children, &pthread->task->threads);
    spin_unlock(&pthread->task->tasklock);
    pthread->rsp = pthread->stack_addr + pthread->stack_size - 8;

    pthread->state = THREAD_READY;
    pthread->on_cpu = oncpu;
    return 0;
}


static int idle_main(ulong arg)
{
    DEBUG_PRINT("idle_main cpu:%d\n",smp_processor_id());
    atomic_inc(&idlenr);
    yield_thread(0);

    for (;;)
        sti_hlt();
    return 0;
}
static int kmain_main(ulong arg)
{
    panic("kmain_main");
    return 0;
}
void goto_idle(void)
{
    DEBUG_PRINT("goto_idle cpu:%d\n",smp_processor_id());

    struct task_struct *task = this_cpu_ptr(&idle_task);
    switch_idle(task->mainthread.stack_addr+task->mainthread.stack_size-8);
    idle_main(0);
}

static void thread_main(struct thread_struct *p, ulong arg)
{
    p->state = THREAD_RUN;
    DEBUG_PRINT("thread_main:%lx,%lx,%s\n", p, arg, p->name);
    while (p->flag & THREAD_SUSPEND)
        yield_thread(arg);
    p->main(arg);
}

ret_t exit_thread(ulong arg)
{
    struct thread_struct *p;
    local_irq_disable();

    struct thread_struct *pold = current;

    p = pold->parent;
    pold->state = THREAD_DONE;
    DEBUG_PRINT("thread %s exist:%lx,status:%d,%lx,%lx\n", pold->name, arg, pold->state, pold,
                p);
    print_regs();
    if (unlikely(!p)) {
        printk("pold->name:%s\n",pold->name);
        panic("no parent thread running\n");
    }
    else {
        if (p->state == THREAD_READY) {

            p->state = THREAD_RUN;
            local_irq_enable();

            return thread_switch_first(p, thread_main, pold, arg);
            //return switch_first(p, thread_main, stack_point, &pold->rsp, arg);                                        //no return

        }
        else {
            p->state = THREAD_RUN;
            local_irq_enable();
            return thread_switch_to(p, pold, arg);
        }
    }
    DEBUG_PRINT("exec exist thread\n");
    for (;;)
        sti_hlt();
}

void wake_up_thread(pthread p)
{
    pthread pold;


    pold = current;
    //DEBUG_PRINT("wake_up thread :%lx,%s,state:%lx,pold:%lx,flag:%lx\n", p, p->name,    p->state, pold, p->flag);

    if (p == pold)
        return;
    if (p->state == THREAD_INIT)
        return;
    if (!(p->flag & THREAD_SUSPEND)) {
#ifdef DEBUG
        bool found = false;

        while (pold) {
            pold = pold->parent;
            if (pold == p) {
                found = true;
            }
        }
        ASSERT(!found);
#endif
        return;
    }
    local_irq_disable();

    if (pold->lvl < p->lvl) {
        ASSERT(p != pold->parent);
        p->parent = pold->parent;
        pold->parent = p;
        local_irq_enable();

        return;
    }

    p->flag &= ~THREAD_SUSPEND;
    set_current(p);
    p->parent = pold;
    if (p->state == THREAD_READY) {

        p->state = THREAD_RUN;
        thread_switch_first(p, thread_main, pold, 0);

    }
    else {
        //p->state = THREAD_RUN;
        thread_switch_to(p, pold, 0);
    }
    local_irq_enable();


}

void suspend_thread()
{
    pthread p;
    local_irq_disable();

    pthread pold = current;

    //DEBUG_PRINT("suspend:%lx\n", pold);
    p = pold->parent;
    ASSERT(p != pold);
    while(p && (p->flag & THREAD_SUSPEND)) {
        p=p->parent;
    }
    if (unlikely(!p)) {
        printk("suspend pold->name:%s\n",pold->name);
        panic("no parent thread running\n");
    }
    else {
        ASSERT(!(pold->flag & THREAD_SUSPEND));
        pold->flag |= THREAD_SUSPEND;
        set_current(p);
        if (p->flag & THREAD_SUSPEND) {
printk("(p->flag & THREAD_SUSPEND) p->name:%s,pold:%s\n",p->name,pold->name);
        }
        if (p->state == THREAD_READY) {

            p->state = THREAD_RUN;
            thread_switch_first(p, thread_main, pold, 0);

        }
        else {
            p->state = THREAD_RUN;
            thread_switch_to(p, pold, 0);
        }

    }
    local_irq_enable();

}
static void sleep_callback (u64 nownsec,void *param)
{

    pthread p = (pthread)param;
    wake_up_thread(p);
}
void nsleep(u64 nsec)
{
    local_irq_disable();
ASSERT(!(current->flag & THREAD_SUSPEND));

    set_timeout_nsec(nsec,sleep_callback,current);
 ASSERT(!(current->flag & THREAD_SUSPEND));

    suspend_thread(); 
}
ret_t resume_thread(pthread p, ulong arg)
{
    pthread pold;
    local_irq_disable();


    if (p->state == THREAD_DONE) {
        DEBUG_PRINT("resume done thread\n");
        ret_t t = { 0, -1 };
        local_irq_enable();

        return t;
    }
    pold = current;
    p->parent = pold;
    if(pold==p)printk("%s\n",p->name);
    ASSERT(pold != p);
    set_current(p);
    if (p->state == THREAD_READY) {

        p->state = THREAD_RUN;
        local_irq_enable();

        return thread_switch_first(p, thread_main, pold, arg);

    }
    else {
        //p->state = THREAD_RUN;
        local_irq_enable();
        return thread_switch_to(p, pold, arg);
    }

}

ret_t yield_thread(ulong arg)
{
    pthread p;
    local_irq_disable();

    ret_t ret;
    pthread pold = current;

    p = pold->parent;
    ASSERT(p != pold);
    if (unlikely(!p)) {
        panic("no parent thread running\n");
    }
    else {
        set_current(p);
        if (p->state == THREAD_READY) {

            p->state = THREAD_RUN;
            ret = thread_switch_first(p, thread_main, pold, arg);

        }
        else {
            p->state = THREAD_RUN;
            ret = thread_switch_to(p, pold, arg);
        }

    }
    local_irq_enable();

    return ret;
}

static __init int init_idle_thread()
{

    __this_cpu_rw_init();
    cpu_p cpu = this_cpu_ptr(&the_cpu);
    struct task_struct *task = this_cpu_ptr(&idle_task);
    memset(task, 0, sizeof(*task));
    INIT_LIST_HEAD(&task->threads);
    task->mainthread.task = task;
    task->mainthread.stack_addr = (ulong) alloc_kheap_4k(IDLE_STACK_SIZE);
    task->mainthread.stack_size = IDLE_STACK_SIZE;
    task->mainthread.main = idle_main;
    task->mainthread.flag = THREAD_IDLE;
    task->mainthread.name = "idle";
    task->mainthread.parent = NULL;
    task->mainthread.real_parent = NULL;
    task->mainthread.lvl = THREAD_LVL_IDLE;
    task->parent = NULL;
    task->real_parent = NULL;

    int ret = create_thread_oncpu(&task->mainthread, cpu->cpu);

    if (ret)
        return ret;
    return 0;
}
static __init int init_kmain_thread()
{
    __this_cpu_rw_init();
    cpu_p cpu = this_cpu_ptr(&the_cpu);
    unmap_free_at((ulong)per_cpu_ptr(&init_stack, cpu->cpu),PAGE_4K_SIZE);
    ulong *p = (ulong *)per_cpu_ptr(&init_stack, cpu->cpu);
    printk("ini_stack_hole:%016lx\n",p);
    
    pthread pmain = this_cpu_ptr(&kmain_thread);
    struct task_struct *task = this_cpu_ptr(&idle_task);
    INIT_LIST_HEAD(&task->threads);
    pmain->task = task;
    pmain->stack_addr = 0;//(ulong) per_cpu_ptr(&init_stack, cpu->cpu);
    pmain->stack_size = KMAIN_STACK_SIZE;//sizeof(init_stack);
    pmain->main = kmain_main;
    pmain->flag = THREAD_PERCPU;
    pmain->name = "kmain";
    pmain->parent = &task->mainthread;
    pmain->real_parent = &task->mainthread;
    pmain->lvl = THREAD_LVL_KMAIN;

    int ret = create_thread_oncpu(pmain, cpu->cpu);

    if (ret)
        return ret;
    set_current( pmain);
printk("current:%lx,pmain:%lx\n",current,pmain);
    pmain->state = THREAD_RUN;
    resume_thread(&task->mainthread, (ulong) 0);
    return 0;
}

__init int static init_thread_call(bool isbp)
{
    init_idle_thread();
    init_kmain_thread();
    return 0;
}

early_initcall(init_thread_call);
