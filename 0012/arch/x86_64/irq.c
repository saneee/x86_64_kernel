#include <asm/cpu.h>
#include <asm/pm64.h>
#include <asm/apic.h>
#include <yaos/irq.h>
#include <yaos/smp.h>
#include <yaos/init.h>
void default_irq_trap(struct trapframe *tf)
{
    irq_enter();
    ((irq_handler_t) (irq_vectors[tf->trapno])) ((int)tf->trapno);
    irq_exit();
//for(;;)cli_hlt();
}

static u64 count = 0;
void default_irq_handler(int n)
{
    if (++count > 1000) {
        printk("irq:%d,cpu:%d\n", n,smp_processor_id());
        count = 0;
    }
    ack_lapic_irq();
}
