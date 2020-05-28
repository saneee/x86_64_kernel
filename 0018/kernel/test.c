#include <types.h>
#include <yaos/init.h>
#include <yaos/time.h>
#include <yaos/printk.h>
#include <yaos/smp.h>
#include <yaos/cpupm.h>
#include <yaos/tasklet.h>
#include <yaos/sched.h>
#include <asm/current.h>
#include <yaoscall/malloc.h>
static int test_init(bool isbp)
{

    if(isbp)return 0;
    char *buf = yaos_malloc(0x1000);
    printk("buf:%p\n",buf);
    yaos_mfree(buf);
    buf = yaos_malloc(0x100000);
    printk("buf 0x100000:%p\n",buf);
    yaos_mfree(buf);    
    return 0;
}


late_initcall(test_init);

