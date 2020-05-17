#ifndef _ASM_X86_PERCPU_H
#define _ASM_X86_PERCPU_H

#ifndef __stringify
#define __stringify_1(x...)     #x
#define __stringify(x...)       __stringify_1(x)
#endif

extern void __bad_percpu_size(void);
#define this_cpu_read_stable(var)       percpu_stable_op("mov", var)
#define percpu_stable_op(op, var)                       \
({                                                      \
        typeof(var) pfo_ret__;                          \
        switch (sizeof(var)) {                          \
        case 1:                                         \
                asm(op "b "__percpu_arg(P1)",%0"        \
                    : "=q" (pfo_ret__)                  \
                    : "p" (&(var)));                    \
                break;                                  \
        case 2:                                         \
                asm(op "w "__percpu_arg(P1)",%0"        \
                    : "=r" (pfo_ret__)                  \
                    : "p" (&(var)));                    \
                break;                                  \
        case 4:                                         \
                asm(op "l "__percpu_arg(P1)",%0"        \
                    : "=r" (pfo_ret__)                  \
                    : "p" (&(var)));                    \
                break;                                  \
        case 8:                                         \
                asm(op "q "__percpu_arg(P1)",%0"        \
                    : "=r" (pfo_ret__)                  \
                    : "p" (&(var)));                    \
                break;                                  \
        default: __bad_percpu_size();                   \
        }                                               \
        pfo_ret__;                                      \
})
#define __percpu_seg gs
#define __percpu_mov_op movq
#define __percpu_prefix         "%%"__stringify(__percpu_seg)":"
#define __percpu_arg(x)         __percpu_prefix "%" #x
#define INIT_PER_CPU_VAR(var)  init_per_cpu__##var
#define DECLARE_INIT_PER_CPU(var) \
       extern typeof(var) init_per_cpu_var(var)
#define init_per_cpu_var(var)  init_per_cpu__##var

#endif
