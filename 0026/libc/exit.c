#include <yaos/printk.h>
void exit(int code)
{
    printk("exit:%d\n",code);
    for(;;);
}
_Noreturn void abort(void)
{
    printk("abort\n");
    for (;;);
}

