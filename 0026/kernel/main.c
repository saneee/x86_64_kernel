#ifndef DEBUG
#define DEBUG
#endif
#include <yaos/types.h>
#include <yaos/init.h>
#include <yaos/kernel.h>
#include <yaos/printk.h>
#include <asm/pgtable.h>
#include <yaos/kheap.h>
#include <asm/cpu.h>
#include <yaos/atomic.h>
#include <yaos/sched.h>
//#include <yaos/sched.h>
#define PRE_SMP_ENDLVL 2
#define MAIN_LVL       8
#if 0
#define DEBUG_PRINT printk
#else
#define DEBUG_PRINT inline_printk

#endif
extern const struct obs_kernel_param __setup_start[], __setup_end[];
extern void smp_init();
static noinline void __init kernel_init_freeable(void);
static void __init do_pre_smp_initcalls(bool isbp);
extern void goto_idle();
extern int nr_cpu;
bool initcall_debug = true;
extern bool can_main_run(pthread p);
atomic_t ap_done_nr = ATOMIC_INIT(0);

static int __ref kernel_init(void *unused)
{
    DEBUG_PRINT("kernel_init start\n");
    kernel_init_freeable();
    return 0;
}

static noinline void __init_refok rest_init(void)
{
    //kernel_thread(kernel_init, PAGE_SIZE, NULL);
    kernel_init(0);
}

static int __init do_early_setup(bool isbp)
{
    const struct obs_kernel_param *p;

    for (p = __setup_start; p < __setup_end; p++) {
        if (p->setup_func(isbp) != 0)
            DEBUG_PRINT("none zero return %p\n", p->setup_func);
    }
    /* We accept everything at this stage. */
    return 0;
}

static int __init do_one_initcall_debug(initcall_t fn, bool isbp)
{
    int ret;

    DEBUG_PRINT("calling  %016lx ,isbp:%d,rsp:%lx\n", fn, isbp, read_rsp());
    ret = fn(isbp);
    DEBUG_PRINT("initcall %016lx returned %d \n", fn, ret);

    return ret;
}

int __init do_one_initcall(initcall_t fn, bool isbp)
{
    int ret;

    if (initcall_debug)
        ret = do_one_initcall_debug(fn, isbp);
    else
        ret = fn(isbp);
    return ret;
}

extern initcall_t __initcallap_start[];
extern initcall_t __initcallap_end[];

extern initcall_t __initcall_start[];
extern initcall_t __initcall0_start[];
extern initcall_t __initcall1_start[];
extern initcall_t __initcall2_start[];
extern initcall_t __initcall3_start[];
extern initcall_t __initcall4_start[];
extern initcall_t __initcall5_start[];
extern initcall_t __initcall6_start[];
extern initcall_t __initcall7_start[];
extern initcall_t __initcall8_start[];
extern initcall_t __initcall_end[];

static initcall_t *initcall_levels[] __initdata = {
    __initcall0_start,
    __initcall1_start,
    __initcall2_start,
    __initcall3_start,
    __initcall4_start,
    __initcall5_start,
    __initcall6_start,
    __initcall7_start,
    __initcall8_start,
    __initcall_end,
};

static void __init do_initcall_level(int level, bool isbp)
{
    initcall_t *fn;

    for (fn = initcall_levels[level]; fn < initcall_levels[level + 1]; fn++)
        do_one_initcall(*fn, isbp);
}

static void __init do_initcalls(bool isbp)
{
    int level;

    for (level = PRE_SMP_ENDLVL; level < MAIN_LVL;
         level++)
        do_initcall_level(level, isbp);
}

static void __init do_basic_setup(bool isbp)
{
    do_initcalls(isbp);
}

static void __init do_pre_smp_initcalls(bool isbp)
{
    initcall_t *fn;

    DEBUG_PRINT("do_pre_smp_initcalls\n");
    for (fn = __initcall_start; fn < __initcall0_start; fn++)
        do_one_initcall(*fn, isbp);
    for (int level = 0; level < PRE_SMP_ENDLVL; level++)
        do_initcall_level(level, isbp);

}

static noinline void __init kernel_init_freeable()
{
    extern const char __init_begin[], __init_end[];

    DEBUG_PRINT("initstart:%lx,end:%lx,size:%lx\n", __init_begin, __init_end,
                __init_end - __init_begin);
    while (atomic_read(&ap_done_nr) != nr_cpu - 1) {
        DEBUG_PRINT("ap_done_nr:%d\n", atomic_read(&ap_done_nr));
    }
    free_kheap_4k((ulong) __init_begin, __init_end - __init_begin);
}
pthread bp_main_thread = 0;
__init static void after_switch_stack(bool isbp)
{
    printk("**********after_switch_stack: isbp:%d, rsp:%lx\n",isbp, read_rsp());
    if(isbp) smp_init();

    do_basic_setup(isbp);
    if(!isbp) {
        printk("ap start done\n");
        atomic_inc(&ap_done_nr);

    }
    if(isbp) rest_init();
    pthread me = current;
    if (isbp) bp_main_thread = me;
    do_initcall_level(MAIN_LVL, isbp);
    if (isbp) {
        for (int i=0;i<100;i++)
        printk("%d,%d,can_main_run(me):%d\n",i,isbp,can_main_run(me));
    } 
    while(1) {
       // if(!can_main_run(me)) nsleep(10000000); 
       can_main_run(me);
    }

}
__init static void switch_stack(bool isbp)
{
   printk("current:%lx\n",current);
   ulong get_phy_addr(ulong);
   ret_t map_paddr_flags_at(ulong);
   ulong stack_top = current->stack_addr+current->stack_size - 2*sizeof(long);
   ret_t ret = map_paddr_flags_at(stack_top);
   ulong paddr = get_phy_addr(stack_top);
   printk("p2v:%lx,read:%lx\n",P2V(paddr),*((ulong *)P2V(paddr)));
   printk("switch to stack:%016lx,from:%016lx,stack_size:%lx,phy_addr:%lx,flags:%lx,p:%lx\n",stack_top,read_rsp(),current->stack_size,get_phy_addr(stack_top),ret.e,ret.v);
   ulong stackpg = stack_top &~0xfff;
   asm volatile("invlpg (%0)" ::"r" (stackpg) : "memory");
 ulong cr4 = read_cr4();
    write_cr4(cr4 ^ cr4_pge);
    write_cr4(cr4);
 
   printk("read stack:%lx\n",*(ulong *)stack_top);
   extern void switch_stack_and_jmp(ulong st,void *pjump,bool isbp);
   switch_stack_and_jmp(stack_top, after_switch_stack, isbp); 
}
__init void start_kernel_ap(void)
{
    void smp_ap_init();
   
    smp_ap_init();
    do_early_setup(false);
    do_pre_smp_initcalls(false);
    switch_stack(false);
}

__init void start_kernel(void)
{
    void arch_setup();

    arch_setup();
    //init_sched(true);
    do_early_setup(true);
    do_pre_smp_initcalls(true);
    //smp_init();
    switch_stack(true);

}

