#include "asm.h"
mb_info = 0x8000
mb_flag = (mb_info)
mb_tsc1_lo = (mb_info + 88)
mb_tsc1_hi = (mb_info + 88 + 4)
mb_tsc_disk_lo = (mb_info + 88 + 8)
mb_tsc_disk_hi = (mb_info + 88 + 12)
mb_cmdline = (mb_info + 16)
mb_mmap_len = (mb_info + 44)
mb_mmap_addr = (mb_info + 48)

e820data = 0x9000

.code16
.globl start
.global hello
start:
 movw   $0x4f02,%ax
 movw   $0x4117,%bx
  .byte 0xcd
  .byte 0x10
  movw   $0x4117,%cx
  xorw 	 %ax,%ax
  movw   %ax,%es
  mov	 %ax,%es
  movw   $0x8400,%di
  movw   $0x4f01,%ax
  .byte  0xcd
  .byte  0x10

do_mb:
#    rdtsc
#    mov %eax, mb_tsc_disk_lo
#    mov %edx, mb_tsc_disk_hi

    mov $e820data, %edi
    mov %edi, mb_mmap_addr
    mov $64, %ebx
    mov %ebx,mb_flag
    xor %ebx, %ebx
more_e820:
    mov $100, %ecx
    mov $0x534d4150, %edx
    mov $0xe820, %ax
    add $4, %edi
    int $0x15
    jc done_e820
    mov %ecx, -4(%edi)
    add %ecx, %edi
    test %ebx, %ebx
    jnz more_e820
done_e820:
    sub $e820data, %edi
     mov %edi, mb_mmap_len
  cli #关中断，bios启动的时候可能开启中断
  xorw    %ax,%ax
  movw    %ax,%ds
  //movw    %ax,%es
  //movw    %ax,%ss
  #打开A20
seta20.1:
  inb     $0x64,%al               # Wait for not busy
  testb   $0x2,%al
  jnz     seta20.1
  movb    $0xd1,%al               # 0xd1 -> port 0x64
  outb    %al,$0x64

seta20.2:
  inb     $0x64,%al               # Wait for not busy
  testb   $0x2,%al
  jnz     seta20.2

  movb    $0xdf,%al               # 0xdf -> port 0x60
  outb    %al,$0x60
  #切换到32位保护模式
  lgdt    gdtdesc
  movl    %cr0, %eax
  orl     $CR0_PE, %eax
  movl    %eax, %cr0
  ljmp    $(SEG_KCODE<<3), $start32

.code32  # Tell assembler to generate 32-bit code now.
start32:
  movw    $(SEG_KDATA<<3), %ax    # Our data segment selector
  movw    %ax, %ds                # -> DS: Data Segment
  movw    %ax, %es                # -> ES: Extra Segment
  movw    %ax, %ss                # -> SS: Stack Segment
  movw    $0, %ax                 # Zero segments not ready for use
  movw    %ax, %fs                # -> FS
  movw    %ax, %gs                # -> GS

  # Set up the stack pointer and call into C.
  movl    $start, %esp
  call    bootmain
entrycall:
  mov	  %eax,%ecx
  mov	  $0x2BADB002,%eax
  mov     $mb_info,%ebx
  jmp *%ecx
  # If bootmain returns (it shouldn't), then loop
spin:
  jmp     spin
.p2align 2                                # force 4 byte alignment
gdt:
  SEG_NULLASM                             # null seg
  SEG_ASM(STA_X|STA_R, 0x0, 0xffffffff)   # code seg
  SEG_ASM(STA_W, 0x0, 0xffffffff)         # data seg

gdtdesc:
  .word   (gdtdesc - gdt - 1)             # sizeof(gdt) - 1
  .long   gdt                             # address gdt
  

