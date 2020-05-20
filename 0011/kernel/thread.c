#include <types.h>
#include <yaos/init.h>
#include <yaos/printk.h>
#include <yaos/percpu.h>
#include <yaos/smp.h>
static int test_early_init(bool isbp)
{
    printk("test_early_init called, isbp：%s, cpu:%d\n",isbp?"true":"false",smp_processor_id());
    return 0;
}
static int test_core_init(bool isbp)
{
    printk("test_core_init called, isbp：%s, cpu:%d\n",isbp?"true":"false",smp_processor_id());
    return 0;

}
static int test_arch_init(bool isbp)
{
    printk("test_arch_init called, isbp：%s, cpu:%d\n",isbp?"true":"false",smp_processor_id());
    return 0;

}

static int test_pure_init(bool isbp)
{
    printk("test_pure_init called, isbp：%s, cpu:%d\n",isbp?"true":"false",smp_processor_id());
    return 0;

}

static int test_device_init(bool isbp)
{
    printk("test_device_init called, isbp：%s, cpu:%d\n",isbp?"true":"false",smp_processor_id());
    return 0;

}
pure_initcall(test_pure_init);
core_initcall(test_core_init);
arch_initcall(test_arch_init);
device_initcall(test_device_init);
early_initcall(test_early_init);



