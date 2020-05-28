#include <asm/percpu.h>
  # vectors.S sends all traps here.
.globl alltraps
alltraps:
  # Build trap frame.
  push %r15
  push %r14
  push %r13
  push %r12
  push %r11
  push %r10
  push %r9
  push %r8
  push %rdi
  push %rsi
  push %rbp
  push %rdx
  push %rcx
  push %rbx
  push %rax

  mov  %rsp, %rdi  # frame in arg1
#  call trap
  movq    120(%rdi), %rax
  movq    c_vectors(,%rax,8),%rax
  call *%rax
  # Return falls through to trapret...
.globl trapret
trapret:
  pop %rax
  pop %rbx
  pop %rcx
  pop %rdx
  pop %rbp
  pop %rsi
  pop %rdi
  pop %r8
  pop %r9
  pop %r10
  pop %r11
  pop %r12
  pop %r13
  pop %r14
  pop %r15

  # discard trapnum and errorcode
  add $16, %rsp
  iretq
.global switchtrap
switchtrap:
  cld
  push %r15
  mov  %rsp,%r15
  incl    PER_CPU_VAR(irq_count)
  jnz 1f #jnz 1f to old version
  movq   PER_CPU_VAR(irq_stack_ptr), %rsp
  sub $256,%rsp
  jmp 2f
3:
  //mov  48(%r15),%rsp
  //sub  $136,%rsp

  push %rdi
  push %rdx
  push %rax
  movq  PER_CPU_VAR(irq_stack_ptr), %rdi
  sub   $256,%rdi
  movq  8(%r15),%rdx #trapno
  xor  %rax,%rax
  movl PER_CPU_VAR(irq_count),%eax
  #call print_regs
  movb %dl,(%rdi,%rax,1)
  pop %rax
  pop %rdx
  pop %rdi
  pop %r15
  add $16,%rsp
  iretq 
1:
  mov  48(%r15),%rsp
  sub  $136,%rsp

2:
  push 56(%r15) #ss
  push 48(%r15) #rsp 
  push 40(%r15) #flag 
  push 32(%r15) #cs
  push 24(%r15)  #ip
  push 16(%r15)
  push 8(%r15)
  push (%r15)   #r15
  push %r14
  push %r13
  push %r12
  push %r11
  push %r10
  push %r9
  push %r8
  push %rdi
  push %rsi
  push %rbp
  push %rdx
  push %rcx
  push %rbx
  push %rax
  mov  %rsp, %rdi  # frame in arg1
  movq    120(%rdi), %rax
  xor %rsi,%rsi
  movq    c_vectors(,%rax,8),%rax

  call *%rax
.global switchtrapret
switchtrapret:
  cli
  decl    PER_CPU_VAR(irq_count)
  #movw $0,PER_CPU_VAR(irq_count)
  pop %rax
  pop %rbx
  pop %rcx
  pop %rdx
  pop %rbp
  pop %rsi
  pop %rdi
  pop %r8
  pop %r9
  pop %r10
  pop %r11
  pop %r12
  pop %r13
  pop %r14
  pop %r15
  add $16,%rsp
  iretq   
.global do_softirq_own_stack
do_softirq_own_stack:
  pushq   %rbp
  mov     %rsp, %rbp
  incl    PER_CPU_VAR(irq_count)
  cmove   PER_CPU_VAR(irq_stack_ptr), %rsp
  push    %rbp                            /* frame pointer backlink */
  call    __do_softirq
  leaveq
  decl    PER_CPU_VAR(irq_count)
  ret

