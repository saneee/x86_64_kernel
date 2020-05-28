/* zhuhanjun 2015/11/23
cpupm struct
*/
#ifndef ARCH_X86_64_CPUPM_H
#define ARCH_X86_64_CPUPM_H 1
#include <yaos/types.h>
#include <yaos/kheap.h>
#include <yaos/printk.h>
#include <asm/cpu.h>
#include <yaos/percpu.h>
//#include <yaos/rett.h>
#define CPU_RUNNING 1
extern ulong __kernel_end;
enum {
    gdt_null = 0,
    gdt_cs = 1,
    gdt_ds = 2,
    gdt_cs32 = 3,
    gdt_tss = 4,
    gdt_tssx = 5,
    nr_gdt
};
struct task_state_segment {
    u32 reserved0;
    u64 rsp[3];
    u64 ist[8];                 // ist[0] is reserved
    u32 reserved1;
    u32 reserved2;
    u16 reserved3;
    u16 io_bitmap_base;
} __attribute__ ((packed));

struct aligned_task_state_segment {
    u32 pad;                    // force 64-bit structures to be aligned
    struct task_state_segment tss;
} __attribute__ ((packed, aligned(8)));

struct arch_cpu {
    struct aligned_task_state_segment atss;

    u32 apic_id;
    u32 acpi_id;
    u64 status;
    u64 rsp;
    u64 gdt[nr_gdt];

};
static inline void set_ist_entry(struct arch_cpu *p, unsigned ist,
                                 unsigned long base)
{
    p->atss.tss.ist[ist] = base;
}

static inline char *get_ist_entry(struct arch_cpu *p, unsigned ist)
{
    return (char *)(p->atss.tss.ist[ist]);
}

void add_int_vector(unsigned n, unsigned ist, void *handler);
extern void bootup_aps(void);   //pm64.c
static inline void arch_start_aps(void)
{
    bootup_aps();
}
#endif
