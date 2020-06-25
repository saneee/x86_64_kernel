#ifndef _YAOS_CPUPM_H
#define _YAOS_CPUPM_H
#include <asm/cpupm.h>
#include <yaos/percpu.h>
struct thread_struct;
struct cpu {
    struct arch_cpu arch_cpu;
    char *percpu_base;
    int cpu;
    unsigned int status;
};
DECLARE_PER_CPU(struct cpu, the_cpu);
typedef struct cpu *cpu_p;
static inline cpu_p arch_cpu_to_cpu(struct arch_cpu *p)
{
    return (cpu_p) ((ulong) p - offsetof(struct cpu, arch_cpu));
}

static inline cpu_p get_current_cpu()
{
    char *base = get_cur_percpu_base();

    return base_to_cpu(base);

}

static inline cpu_p get_cpu(int oncpu)
{
    char *base = get_percpu_base(oncpu);

    return base_to_cpu(base);
}

static inline bool is_bp()
{
    cpu_p p = get_current_cpu();

    return p->cpu == 0;
}
#endif
