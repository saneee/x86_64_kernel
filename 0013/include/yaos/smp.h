#ifndef _YAOS_SMP_H
#define _YAOS_SMP_H
#include <yaos/percpu.h>
#define raw_smp_processor_id()  this_cpu_number()

#define smp_processor_id() raw_smp_processor_id()

struct smp_percpu_thread {
    struct task_struct *task;
    struct list_head list;
    int (*thread_should_run) (unsigned int cpu);
    int (*thread_fn) (unsigned long cpu);
    void (*create) (unsigned int cpu);
    void (*setup) (unsigned int cpu);
    const char *thread_comm;
    unsigned int lvl;
    unsigned int cpu;
    unsigned int stack_size;
};

int smpboot_register_percpu_thread(struct smp_percpu_thread *plug_thread);

#endif

