#ifndef _ASM_X86_CURRENT_H
#define _ASM_X86_CURRENT_H
#include <yaos/compiler.h>
#include <asm/percpu.h>
struct thread_t;
DECLARE_PER_CPU(struct thread_t *, current_thread);
static __always_inline struct thread_t *get_current(void)
{
        return this_cpu_read_stable(current_thread);
}
#define current get_current()
#endif
