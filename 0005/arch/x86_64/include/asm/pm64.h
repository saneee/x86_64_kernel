/* zhuhanjun 2015/10/29 
per cpu struct
*/
#ifndef ARCH_X86_64_PM64_H
#define ARCH_X86_64_PM64_H 1
#include <yaos/types.h>
#include <yaos/string.h>
#include <yaos/kheap.h>
#include <yaos/printk.h>
#include <asm/cpu.h>
#define INIT_STACK_SIZE 4096
#define EXCEPTION_STACK_SIZE 4096
#define INTERRUPT_STACK_SIZE 4096
#define NR_EXCEPTION_NEST 3
#define CPU_RUNNING 1
extern ulong __kernel_end;
extern ulong __percpu_start;
extern ulong __percpu_size;
static inline char *new_percpu()
{
    char *p= kalloc(__percpu_size);
printk("memcpy:%lx,%lx,%lx\n",p,__percpu_start,__percpu_size);
    memcpy(p,(char *)__percpu_start,__percpu_size);
    return p;
}
extern struct arch_cpu the_cpu;
static inline struct arch_cpu * base_to_cpu(char * base)
{
   return (struct arch_cpu *)(((ulong)(&the_cpu))+(ulong)base-(__percpu_start));
}
static inline ulong base_to_percpu(char* base,void *p)
{
   return ((ulong)p+(ulong)base-(__percpu_start));
}
extern char *cpu_base[];
static inline ulong cpuid_to_percpu(int cpu,void *p)
{
    return ((ulong)p+(ulong)cpu_base[cpu]-__percpu_start);
}
static inline ulong get_percpu_valp(void *p)
{
    
    int apicid = cpuid_apic_id();
    return cpuid_to_percpu(apicid,p);
}

static inline struct arch_cpu * get_current_cpu()
{
    int apicid = cpuid_apic_id();
    return (struct arch_cpu *)cpuid_to_percpu(apicid,&the_cpu);

}
enum {
    gdt_null = 0,
    gdt_cs = 1,
    gdt_ds = 2,
    gdt_cs32 =3,
    gdt_tss =4,
    gdt_tssx =5,
    nr_gdt
};
struct task_state_segment {
    u32 reserved0;
    u64 rsp[3];
    u64 ist[8];   // ist[0] is reserved
    u32 reserved1;
    u32 reserved2;
    u16 reserved3;
    u16 io_bitmap_base;
} __attribute__((packed));

struct aligned_task_state_segment {
    u32 pad;  // force 64-bit structures to be aligned
    struct task_state_segment tss;
} __attribute__((packed, aligned(8)));

struct trapframe {
  u64 rax;      
  u64 rbx;
  u64 rcx;
  u64 rdx;
  u64 rbp;
  u64 rsi;
  u64 rdi;
  u64 r8;
  u64 r9;
  u64 r10;
  u64 r11;
  u64 r12;
  u64 r13;
  u64 r14;
  u64 r15;

  u64 trapno;
  u64 err;

  u64 rip;     // rip
  u64 cs;
  u64 eflags;  // rflags
  u64 rsp;     // rsp
  u64 ds;      // ss
};
struct cpu_stack;
struct cpu_stack {
    char init_stack[INIT_STACK_SIZE] __attribute__((aligned(16)));
    char interrupt_stack[INTERRUPT_STACK_SIZE] __attribute__((aligned(16)));
    char exception_stack[EXCEPTION_STACK_SIZE*NR_EXCEPTION_NEST] __attribute__((aligned(16)));
} __attribute__((packed));

struct thread_t;
struct arch_cpu{
    struct aligned_task_state_segment atss;
    struct cpu_stack stack;

    u32 apic_id;
    u32 acpi_id;
    u64 status;
    u64 rsp;
    char *percpu_base;
    struct thread_t *current_thread;
    u64 gdt[nr_gdt];

};
static inline void set_ist_entry(struct arch_cpu *p,unsigned ist,char *base,size_t size)
{
    p->atss.tss.ist[ist]=(u64)(base+size);	
}
static inline char* get_ist_entry(struct arch_cpu *p,unsigned ist)
{
    return (char *)(p->atss.tss.ist[ist]);
}
static inline void set_exception_stack(struct arch_cpu *p)
{
    char *base = p->stack.exception_stack;
    set_ist_entry(p,1, base, EXCEPTION_STACK_SIZE*NR_EXCEPTION_NEST);
}
static inline void set_interrupt_stack(struct arch_cpu *p)
{
    char *base = p->stack.interrupt_stack;
    set_ist_entry(p,2, base, INTERRUPT_STACK_SIZE);
}
static inline ulong cpu_percpu_valp(struct arch_cpu *pcpu,void *p)
{
     return ((ulong)p+(ulong)pcpu->percpu_base-(__percpu_start));
}

void add_int_vector(unsigned n,unsigned ist,void *handler);

#endif
