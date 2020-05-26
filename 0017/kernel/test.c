#include <types.h>
#include <yaos/init.h>
#include <yaos/time.h>
#include <yaos/printk.h>
#include <yaos/smp.h>
#include <yaos/cpupm.h>
#include <yaos/tasklet.h>
#include <yaos/sched.h>
#include <asm/current.h>
static int test_init(bool isbp)
{
    if(!isbp)return 0;
    char *buf = alloc_kheap_4k(0x1000);
    void ide_read(u32 sector, void *data, u8 cnt, bool head);
    void ide_write(u32 sector, void *data, u8 cnt, bool head);
    memset(buf,0,512);
    ide_read(0,buf,1,true);
    nsleep(40000);
    dump_mem(buf,512);    
    memset(buf,'H',512);
    ide_write(1,buf,1,true);
    memset(buf,0,512);
    ide_read(1,buf,1,true);
    dump_mem(buf,512);
    return 0;
}


late_initcall(test_init);

