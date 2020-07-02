#include <yaos/printk.h>
#include <asm/pm64.h>
#include <asm/pgtable.h>
#include <yaos/sched.h>
#include <yaos/vm.h>
#if 1
#define DEBUG_PRINT printk
#else
#define DEBUG_PRINT inline_printk
#endif
DEFINE_PER_CPU(int, except_count6) __visible = -1;
DEFINE_PER_CPU(int, except_count7) __visible = -1;
DEFINE_PER_CPU(int, except_count8) __visible = -1;
DEFINE_PER_CPU(int, except_count9) __visible = -1;

DEFINE_PER_CPU(int, except_count) __visible = -1;
DEFINE_PER_CPU(int, except_count2) __visible = -1;
DEFINE_PER_CPU(int, except_count3) __visible = -1;
DEFINE_PER_CPU(int, except_counti4) __visible = -1;

DEFINE_PER_CPU(struct trapframe *, first_tf) __visible = 0;
extern void cprint_regs(struct trapframe *tf);
extern char *pg_stack_ptr;
struct idt_entry {
    u16 offset0;
    u16 selector;
    u8 ist:3;
    u8 res0:5;
    u8 type:4;
    u8 s:1;
    u8 dpl:2;
    u8 p:1;
    u16 offset1;
    u32 offset2;
    u32 res1;
} __attribute__ ((aligned(16)));
extern struct idt_entry _idt[256];

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
static inline bool is_irq_stack_inc(ulong addr)
{
    ulong top = (ulong)this_cpu_read(irq_stack_ptr);
    return (addr<top && addr>top-PAGE_SIZE_LARGE);
}
static inline bool is_stack_inc(ulong addr)
{
    return check_stack(addr);
    pthread p=current;
    return (p && p->stack_addr < addr && p->stack_addr + p->stack_size > addr) 
            || is_irq_stack_inc(addr);
    
}
static void map_page(ulong cr2)
{
    ulong alloc_small_phy_page_safe();
    ulong paddr = alloc_small_phy_page_safe();
    if (!paddr)panic("no memory to map page fault\n");
    map_4k_page_p2v(paddr, cr2&~0xfff, map_flags_stack());
}
void static write_rsp(ulong r)
{
    asm volatile ("mov %0, %%rsp"::"r" (r));

}
void check_error_stack(ulong rsp, ulong r15)
{
    printk("rsp:%016lx, r15:%16lx,%d,cr2:%lx\n",rsp,r15,this_cpu_read(except_count),read_cr2());
    dump_stack();
    ulong *p =(ulong *)r15;
    printk("%lx,%lx,%lx,\n%lx,%lx,%lx\n%lx,%lx",p[0],p[1],p[2],p[3],p[4],p[5],p[6],p[7]);
    asm volatile("cli");
    for(;;);

}
void page_fault_handler(struct trapframe *tf)
{
    int ecount;    
    ulong cr2 =  read_cr2();
    ulong rsp = read_rsp();
    ulong pg_stack =  (ulong)this_cpu_read(pg_stack_ptr);
    if (rsp>pg_stack || rsp< pg_stack - PAGE_SIZE_LARGE) {
        write_rsp(pg_stack);
        printk("not in pg fault but called page_fault_handler,stack:%lx\n",rsp);
        printk("current:%s,cr2:%lx\n",current->name,cr2);
        cpu_p pcpu = this_cpu_ptr(&the_cpu);
        for (int i=0;i<8;i++)
           printk("ist:%d,%016lx\n",i,pcpu->arch_cpu.atss.tss.ist[i]);

        struct idt_entry e = _idt[0xe];
        printk("e.ist:%d,e.selector:%d,offset:%lx,%lx,%lx\n",e.ist, e.selector, e.offset0, e.offset1, e.offset2);
        dump_stack();
        find_stack(rsp);
        cprint_regs(tf);
        asm volatile("cli");
        for(;;);

    }
    if((ecount=this_cpu_read(except_count))!=0){
        struct trapframe *first =this_cpu_read(first_tf);
        printk("double exception pagefault,%d,now:%lx,first:%lx,irq_count:%d\n",ecount,read_rsp(),first,this_cpu_read(irq_count));
        if (is_stack_inc(cr2)) {
           printk("cr2:%lx,rip:%lx\n",cr2,tf->rip);
           map_page(cr2);
           DEBUG_PRINT("@@@@inc stack at:%lx\n",cr2);
           return;
        } else printk("is_stack_inc false:%lx\n",cr2);

 
    }// else this_cpu_write(first_tf,tf);
    ASSERT(tf->trapno == 0xe);
    
    if (is_stack_inc(cr2)) {
       printk("cr2:%lx,rip:%lx\n",cr2,tf->rip);

       map_page(cr2);
       DEBUG_PRINT("@@@@inc stack at:%lx\n",cr2);
       return;
    }else printk("is_stack_inc false:%lx\n",cr2); 

    asm volatile("cli");
    dump_stack();
    find_stack(tf->rsp);
    cprint_regs(tf);
    asm volatile("cli");
    for(;;);

}
void default_exception(struct trapframe *tf)
{
    /*
    int ecount;
    if((ecount=this_cpu_read(except_count))!=0){
        struct trapframe *first =this_cpu_read(first_tf);
        printk("double exception,%d,now:%lx,first:%lx\n",ecount,read_rsp(),first);
        cprint_regs(tf);
        if(first)cprint_regs(first);
    }// else this_cpu_write(first_tf,tf);
    */
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
