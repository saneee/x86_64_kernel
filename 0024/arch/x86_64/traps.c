#include <yaos/printk.h>
#include <asm/pm64.h>
#include <asm/pgtable.h>
#include <yaos/sched.h>
#if 1
#define DEBUG_PRINT printk
#else
#define DEBUG_PRINT inline_printk
#endif

void jump_to_zero()
{
    print_regs();
    printk("jump to zero?\n");
    asm volatile("cli");
    for(;;);
}
void install_zero_hook()
{
    uchar code[20];
    ulong addr = (ulong) jump_to_zero - 5; //相对偏移跳转，减去指令长度5字节
    uchar *ptr = (uchar *) 0;
    printk("addr %lx\n",addr);
    code[0] = 0xe9;
    code[1] = addr & 0xff;
    code[2] = (addr >> 8) & 0xff;
    code[3] = (addr >> 16) & 0xff;
    code[4] = (addr >> 24) & 0xff;
    for (int i = 0; i < 5; i++) {
        printk(" %x ",code[i]);
        *ptr++ = code[i];
    }
    printk("install_zero_hook：%lx\n", (ulong) jump_to_zero);
}
static inline bool is_stack_inc(ulong addr)
{
    pthread p=current;
    return (p && p->stack_addr < addr && p->stack_addr + p->stack_size > addr);
    
}
void page_fault_handler(struct trapframe *tf)
{
    ASSERT(tf->trapno == 0xe);
    ulong cr2 =  read_cr2();
    if (is_stack_inc(cr2)) {
       map_alloc_at(cr2&~0xfff,PAGE_4K_SIZE,map_flags_stack());
       DEBUG_PRINT("@@@@inc stack at:%lx\n",cr2);
       return;
    } 
    extern void cprint_regs(struct trapframe *tf);
    cprint_regs(tf);
    asm volatile("cli");
    for(;;);

}
void default_exception(struct trapframe *tf)
{
    if(tf->trapno==6) {
       ulong cr0 = read_cr0();
       cr0 &= ~7;
       cr0 |=2;
       write_cr0(cr0);
       printk("new cr0:%x\n",read_cr0());
       return;
    }
    if (tf->trapno==0xe) {
       ulong cr2 = read_cr2();
       ret_t ret = map_paddr_flags_at(cr2);

       printk("map addr:%lx,flags:%lx\n",ret.v,ret.e);
       
    }
    extern void cprint_regs(struct trapframe *tf);
    cprint_regs(tf);
    asm volatile("cli");
    for(;;);

}
void default_syscall(struct trapframe *tf)
{
    printk("SSYSCALL EIP:%lx,Error:%lx\n", tf->rip, tf->trapno);
    print_regs();

}
void default_trap(struct trapframe *tf)
{
    printk("Trap EIP:%lx,Error:%lx\n", tf->rip, tf->trapno);
    print_regs();

}
