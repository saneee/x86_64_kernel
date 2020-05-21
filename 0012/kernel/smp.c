#include <types.h>
#include <asm/pgtable.h>
#include <yaos/kheap.h>
#include <errno.h>
#include <yaos/smp.h>
#include <yaos/cpupm.h>
#include <string.h>
int nr_cpu = 0;
char *new_percpu()
{
    char *p = kalloc((__per_cpu_end - __per_cpu_start));

    memcpy(p, (char *)__per_cpu_load, (__per_cpu_end - __per_cpu_start));
    return p;
}

void smp_init()
{

    arch_start_aps();

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
//    list_add(&plug_thread->list, &percpu_threads);
    return ret;
}


