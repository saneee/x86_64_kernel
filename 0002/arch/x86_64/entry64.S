#define mboot_magic 0x1badb002
#define mboot_flags 0x00010040

.code32 #加载程序已经进入32位保护模式，直接从32位开始进入64位长模式
.global mboot_header
.global mboot_entry

mboot_header: #启动头结构，用于qemu加载
  .long mboot_magic
  .long mboot_flags
  .long (-mboot_magic -mboot_flags)     # checksum
  .long mboot_load_addr
  .long mboot_load_addr
  .long mboot_load_end
  .long mboot_bss_end
  .long mboot_entry_addr
mboot_entry:
# multiboot_info_type in ebx
  push %ebx
  push %eax
# zero 4 pages for our bootstrap page tables
  xor %eax, %eax
  mov $0x1000, %edi
  mov $0x5000, %ecx
  rep stosb
#建立1G内存页表用于初始化
# P4ML[0] -> 0x2000 (PDPT-A)
  mov $(0x2000 | 3), %eax
  mov %eax, 0x1000

# P4ML[511] -> 0x3000 (PDPT-B)
  mov $(0x3000 | 3), %eax
  mov %eax, 0x1FF8

# PDPT-A[0] -> 0x4000 (PD)
  mov $(0x4000 | 3), %eax
  mov %eax, 0x2000

# PDPT-B[510] -> 0x4000 (PD)
  mov $(0x4000 | 3), %eax
  mov %eax, 0x3FF0

# PD[0..511] -> 0..1022MB
  mov $0x83, %eax
  mov $0x4000, %ebx
  mov $512, %ecx
ptbl_loop:
  mov %eax, (%ebx)
  add $0x200000, %eax
  add $0x8, %ebx
  dec %ecx
  jnz ptbl_loop
  xor %ebx, %ebx #ebx清0，主CPU从entry启动，其他cpu从entry32mp启动，ebx0表示主CPU
  pop %esi #restore magic
  pop %edi #restore multiboot_info_type
.global entry32mp
entry32mp:
# CR3 -> 0x1000 (P4ML)
  mov $0x1000, %eax
  mov %eax, %cr3

  lgdt (gdtr64 - mboot_header + mboot_load_addr)

# Enable PAE - CR4.PAE=1
  mov %cr4, %eax
  bts $5, %eax
  mov %eax, %cr4

# enable long mode - EFER.LME=1,进入64位长模式
  mov $0xc0000080, %ecx
  rdmsr
  bts $8, %eax
  wrmsr

# enable paging
  mov %cr0, %eax
  bts $31, %eax
  mov %eax, %cr0

# shift to 64bit segment
  ljmp $8,$(entry64low - mboot_header + mboot_load_addr)
.align 16
gdtr64:
  .word gdt64_end - gdt64_begin - 1;
  .quad gdt64_begin - mboot_header + mboot_load_addr

.align 16
gdt64_begin:
  .long 0x00000000 # 0: null desc
  .long 0x00000000
  .long 0x00000000 # 1: Code, R/X, Nonconforming
  .long 0x00209800
  .long 0x00000000 # 2: Data, R/W, Expand Down
  .long 0x00009000
gdt64_end:

.align 16
.code64
entry64low: #进入64位代码
  movq $entry64high, %rax
  jmp *%rax
.global _start
_start:
entry64high:
# ensure data segment registers are sane
  xor %rax, %rax
  mov %ax, %ss
  mov %ax, %ds
  mov %ax, %es
  mov %ax, %fs
  mov %ax, %gs

# check to see if we're booting a secondary core
  test %ebx, %ebx
  #jnz entry64mp 暂时只处理主cpu
  movq    $the_cpu+4208, %rax #the_cpu为每个cpu的栈地址
  mov %rax,%rsp
  jmp bp_main

.global __deadloop
__deadloop:
# we should never return here...
  jmp .

