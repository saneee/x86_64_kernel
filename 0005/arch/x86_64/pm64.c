#define MAX_CPUS        1024
#include <asm/cpu.h>
#include <asm/pm64.h>
#include <yaos/percpu.h>
#include <yaos/string.h>
//cpu数据，每个cpu各自一份，主cpu使用the_cpu，其他cpu会复制.percpu段。
PERCPU(struct arch_cpu, the_cpu);
char *cpu_base[MAX_CPUS]; //每个cpu的隐私数据起始地址。

