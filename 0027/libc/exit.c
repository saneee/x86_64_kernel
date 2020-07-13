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
void __assert_fail(const char *expr, const char *file, int line, const char *func)
{
        printk("Assertion failed: %s (%s: %s: %d)\n", expr, file, func, line);
        abort();
}

