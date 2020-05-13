#ifndef __YAOS_PERCPU_H
#define __YAOS_PERCPU_H
#include <asm/percpu.h>

#ifndef PERCPU
#define PERCPU(type, var) type var __attribute__((section(".percpu"))) 
#endif
#ifndef __percpu_data
#define __percpu_data __attribute__((section(".percpu")))

#endif

#endif
