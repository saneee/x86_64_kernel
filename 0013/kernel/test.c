#include <types.h>
#include <yaos/init.h>
#include <yaos/time.h>
#include <yaos/printk.h>
#include <yaos/smp.h>
static void test (u64 nowmsec) 
{
    printk("test timeout: %lx,cpu:%d\n",nowmsec,smp_processor_id());
}
static int test_init(bool isbp)
{
    set_timeout(11000,test);
    return 0;

}
late_initcall(test_init);

