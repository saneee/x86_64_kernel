#include <types.h>
#include <yaos/sched.h>
#include <asm/current.h>
int *__errno_location(void)
{
    return &current->errno;
}

