#ifndef __YAOS_PERCPU_H
#define __YAOS_PERCPU_H
#include <string.h>
#include <yaos/printk.h>
#include <asm/percpu.h>
#include <yaos/kheap.h>
#include <yaos/compiler.h>
#include <yaos/percpu_defs.h>
#define DEFINE_PERCPU(type,name) DEFINE_PER_CPU(type,name)
#define DEFINE_PERCPU_FIRST(type,var)  DEFINE_PER_CPU_SECTION(type, var, ".first")
#define DEFINE_PERCPU_LAST(type,var) DEFINE_PER_CPU_SECTION(type, var, ".last")
#ifndef __percpu_data
#define __percpu_data __attribute__((section(".percpu")))

#endif
#define per_cpu_offset(x) ((unsigned long)cpu_base[x])
extern unsigned char  _percpu_start[];
extern unsigned char  _percpu_end[];
extern unsigned char  _percpu_size[];
extern unsigned char  __per_cpu_load[];
extern unsigned char  _kernel_start[];
extern unsigned char  _kernel_end[];
extern unsigned char  init_percpu_start[];
extern unsigned char  init_percpu_end[];
extern unsigned char  __per_cpu_start[];
extern unsigned char  __per_cpu_end[];
extern unsigned char  __per_cpu_size[];
extern char *cpu_base[];
extern int nr_cpu;
DECLARE_PER_CPU(int, cpu_number);
DECLARE_PER_CPU(struct cpu, the_cpu);
DECLARE_INIT_PER_CPU(the_cpu);
extern char *new_percpu();

static inline char *get_percpu_base(int cpu)
{
    return cpu_base[cpu];
}
static inline struct cpu *base_to_cpu(char *base)
{
    return (struct cpu *)(((ulong) (&the_cpu)) + (ulong) base -
                          (ulong) (__per_cpu_start));
}

static inline ulong base_to_percpu(char *base, void *p)
{
    return ((ulong) p + (ulong) base - (ulong) (__per_cpu_start));
}
#define BASE_TO_PERCPU(base,p) \
    (typeof p)base_to_percpu(base,p)
static inline ulong cpuid_to_percpu(int cpu, void *p)
{
    return ((ulong) p + (ulong) cpu_base[cpu] - (ulong) __per_cpu_start);
}

#define CPUID_TO_PERCPU(cpu,p) \
      (typeof p)cpuid_to_percpu(cpu,p)

static inline ulong get_percpu_valp(void *p)
{

    ulong base = _get_cur_percpu_base();

    return base_to_percpu((char *)base, p);
}
static inline int get_cpu_number()
{
    char *base = (char *)_get_cur_percpu_base();

    for (int i = 0; i < nr_cpu; i++) {
        if (base == cpu_base[i])
            return i;
    }
    panic("can't get cpu_number:%lx\n", base);
    return 0;
}

static inline void *get_cur_percpu_base()
{
    return (void *)_get_cur_percpu_base();
}

#ifndef arch_this_cpu_number
static inline int this_cpu_number()
{
    return *((int *)get_percpu_valp(&cpu_number));
}
#else
static inline int this_cpu_number()
{
    return arch_this_cpu_number();
}

#endif

#define PERCPU_PTR(p) (typeof(p))(get_percpu_valp(p))
#define __this_cpu_rw_init() do{}while(0)
#define PER_CPU_PTR(ptr, cpu)                                           \
({                                                                      \
        __verify_pcpu_ptr(ptr);                                         \
        SHIFT_PERCPU_PTR((ptr),  (ulong)cpu->percpu_base);                \
})
#define PER_CPU_PTR_VAR(ptr,percpu) (*(PER_CPU_PTR(ptr, percpu)))

static inline ulong _per_cpu_ptr(void *p, int cpu)
{
    char *base = cpu_base[cpu];
    return base_to_percpu(base, p);
}
static inline bool isbpcpu(struct cpu *p) {
    return p==&INIT_PER_CPU_VAR(the_cpu);
}
static inline bool isbp()
{
    return this_cpu_read(cpu_number)==0;
}
#define for_each_possible_cpu(cpu) \
    for(cpu=0;cpu<nr_cpu;cpu++)

#endif

