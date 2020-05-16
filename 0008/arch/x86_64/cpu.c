#include <asm/cpu.h>
#include <types.h>
#include <yaos/printk.h>
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
void cprint_regs(struct trapframe *tf)
{
    ulong rsp=tf->rbp;
    ulong *p;
    printk("CS:%x,apic:%d,cr3:%lx\n", read_cs(),cpuid_apic_id(),read_cr3());
    printk("TR:%x ", read_tr());
    printk("RSP:%lx\n",rsp);
printk("RAX:%lx,RBX:%lx,RCX:%lx\n",tf->rax,tf->rbx,tf->rcx);
printk("RDX:%lx,RSI:%lx,RDI:%lx\n",tf->rdx,tf->rsi,tf->rdi);
    p=(ulong *)rsp;
    dump_mem(p,128);
}

