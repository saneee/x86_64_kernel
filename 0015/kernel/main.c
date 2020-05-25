#ifndef DEBUG
#define DEBUG
#endif
#include <yaos/types.h>
#include <yaos/init.h>
#include <yaos/kernel.h>
#include <yaos/printk.h>
#include <asm/pgtable.h>
#include <yaos/kheap.h>
#include <yaos/atomic.h>
//#include <yaos/sched.h>
#define PRE_SMP_ENDLVL 2
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
    goto_idle();
    panic("idle_main should never return \n");
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

    DEBUG_PRINT("calling  %pF ,isbp:%d\n", fn, isbp);
    ret = fn(isbp);
    DEBUG_PRINT("initcall %pF returned %d \n", fn, ret);

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

    for (level = PRE_SMP_ENDLVL; level < ARRAY_SIZE(initcall_levels) - 1;
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
    extern atomic_t idlenr;

    DEBUG_PRINT("initstart:%lx,end:%lx,size:%lx\n", __init_begin, __init_end,
                __init_end - __init_begin);
    while (atomic_read(&idlenr) != nr_cpu - 1) {
        DEBUG_PRINT("idlenr:%d\n", atomic_read(&idlenr));
    }
    free_kheap_4k((ulong) __init_begin, __init_end - __init_begin);
}

__init void start_kernel_ap(void)
{
    void smp_ap_init();
   
    smp_ap_init();
    do_early_setup(false);
    do_pre_smp_initcalls(false);
    do_basic_setup(false);
    printk("ap start done\n");
    //extern void print_regs();
    //print_regs();
    goto_idle();

}

__init void start_kernel(void)
{
    void arch_setup();

    arch_setup();
    //init_sched(true);
    do_early_setup(true);
    do_pre_smp_initcalls(true);
    smp_init();
    do_basic_setup(true);

    rest_init();
}

