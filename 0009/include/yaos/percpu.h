#ifndef __YAOS_PERCPU_H
#define __YAOS_PERCPU_H
#include <asm/percpu.h>
#include <yaos/compiler.h>

#define DEFINE_PERCPU(type,name) DEFINE_PER_CPU(type,name)
#define DEFINE_PERCPU_FIRST(type,var)  DEFINE_PER_CPU_SECTION(type, var, ".first")
#define DEFINE_PERCPU_LAST(type,var) DEFINE_PER_CPU_SECTION(type, var, ".last")
#ifndef __percpu_data
#define __percpu_data __attribute__((section(".percpu")))

#endif
#define DECLARE_PER_CPU(type, name)                                     \
        DECLARE_PER_CPU_SECTION(type, name, "")

#define DEFINE_PER_CPU(type, name)                                      \
        DEFINE_PER_CPU_SECTION(type, name, "")
#define DECLARE_PER_CPU_SECTION(type, name, sec)                        \
        extern  __PCPU_ATTRS(sec) __typeof__(type) name
#define DEFINE_PER_CPU_SECTION(type, name, sec)                         \
        __PCPU_ATTRS(sec) __typeof__(type) name
#define __PCPU_ATTRS(sec)                                               \
        __percpu __attribute__((section(PER_CPU_BASE_SECTION sec)))     \
        PER_CPU_ATTRIBUTES
#define PER_CPU_BASE_SECTION ".percpu"
#ifndef PER_CPU_ATTRIBUTES
#define PER_CPU_ATTRIBUTES
#endif

#endif
