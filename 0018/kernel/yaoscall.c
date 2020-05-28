#include <yaos/types.h>
#include <yaos/printk.h>
#include <yaos/module.h>
#include <yaos/sysparam.h>
#include <yaos/yaoscall.h>
#include <yaos/init.h>

static ret_t dummy(ulong arg1, ...)
{
    ret_t ret = { 0, ERR_YAOSCALL_NONE_EXIST };
    printk("none exist yaoscall:%lx\n", arg1);
    return ret;
}

static ret_t dummy1(ulong arg1, ...)
{
    ret_t ret = { 0, ERR_YAOSCALL_NONE_EXIST };
    printk("none exist yaoscall:%lx\n", arg1);
    return ret;
}

DECLARE_YAOSCALL(0, dummy1);
DECLARE_YAOSCALL(0, dummy);

static inline ulong myabs(ulong a, ulong b)
{
    return a > b ? a - b : b - a;
}

__init int init_yaoscall(bool isbp)
{
    if (!isbp)
        return 0;
    extern struct yaoscall_st _yaoscall_data_start[];	//kernel64.ld
    extern struct yaoscall_st _yaoscall_data_end[];
    struct yaoscall_st *p = _yaoscall_data_start;
    ulong step =
        myabs((ulong) GET_YAOSCALL(0, dummy1), (ulong) GET_YAOSCALL(0, dummy));

    int i;

    for (i = 0; i < MAX_YAOSCALL; i++) {
        pyaoscall_table[i] = dummy;
    }
    while (p < _yaoscall_data_end) {
        pyaoscall_table[p->id] = p->func;
        printk("regist yaoscall:%d,%lx\n", p->id, p->func);
        p = (struct yaoscall_st *)((ulong) (p) + step);
    }
    return 0;
}

early_initcall(init_yaoscall);
