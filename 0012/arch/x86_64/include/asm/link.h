#ifndef _ASM_X86_LINK_H
#define _ASM_X86_LINK_H
extern unsigned char  _percpu_start[];
extern unsigned char  _percpu_end[];
extern unsigned char  _percpu_size[];
extern unsigned char  __per_cpu_load[];
extern unsigned char  _kernel_start[];
extern unsigned char  _kernel_end[];
extern unsigned char  init_percpu_start[];
extern unsigned char  init_percpu_end[];
#define __percpu_size (unsigned long )_percpu_size
#define __percpu_start (unsigned long )_percpu_start
#define __percpu_end (unsigned long)_percpu_end
#define __kernel_start (unsigned long)_kernel_start
#define __kernel_end (unsigned long)_kernel_end
#endif

