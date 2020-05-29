#include <asm/cpu.h>
#include <asm/pm64.h>
#include <asm/apic.h>
#include <yaos/irq.h>
#include <yaos/smp.h>
#include <yaos/init.h>

void default_irq_trap(struct trapframe *tf)
{
    uchar *ptraps = (uchar *)(this_cpu_read(irq_stack_ptr)-256);
// printk("traps %016lx:%d,%d\n",ptraps,tf->trapno,ptraps[this_cpu_read(irq_count)]);
    irq_enter();
    ((irq_handler_t) (irq_vectors[tf->trapno])) ((int)tf->trapno);
    irq_exit();
    return;
    for (int i=1;i<=this_cpu_read(irq_count);i++) {
        tf->trapno = (ulong)ptraps[i];
        irq_enter();
        ((irq_handler_t) (irq_vectors[tf->trapno])) ((int)tf->trapno);
        irq_exit();
        printk("trapno:%d,i:%d,irq_count:%d\n",tf->trapno,i,this_cpu_read(irq_count));
    }
 //   printk("this_cpu_read(irq_count):%d\n",this_cpu_read(irq_count));
  //  dump_mem(ptraps,0x100);
//for(;;)cli_hlt();
}

static u64 count = 0;
void default_irq_handler(int n)
{
    if (++count > 0) {
        printk("****default_irq_handler**** irq:%d,cpu:%d\n", n,smp_processor_id());
        count = 0;
    }
    ack_lapic_irq();
}
