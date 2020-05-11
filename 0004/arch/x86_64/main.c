#include <yaos/printk.h>
#include <types.h>
extern void print_regs();
extern void uart_early_init();
//the_cpu为每个cpu的数据，目前仅用于cpu栈
char the_cpu[4096] __attribute__((section(".percpu")));
void bp_main(void)
{
  char *w="Hello World!";
  u64 *p = (u64*)0xb8000;
  int i;
  uart_early_init();
  for(i=0;i<500;i++)*p++=0x0720072007200720;
  printk("yaos printk:%s\n",w);
  print_regs();
  dump_mem((char*)0xc0000,0x40);
  for(;;);
}
