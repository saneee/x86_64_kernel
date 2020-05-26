#ifndef _SYSCALL_MODULE_H
#define _SYSCALL_MODULE_H
#include <yaos/syscall.h>
#define SYSCALL_MODULE_FIND_BY_NAME (0<<32)
static inline ret_t syscall_module(ulong arg1,ulong arg2)
{
    return yaos_syscall(SYSCALL_MODULE,arg1,arg2);
}
static inline ret_t syscall_module_find_by_name(const char *name)
{
    return yaos_syscall(SYSCALL_MODULE|MODULE_FIND_BY_NAME,(ulong)name);
}
#endif
