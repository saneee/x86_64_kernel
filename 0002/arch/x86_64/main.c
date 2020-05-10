//the_cpu为每个cpu的数据，目前仅用于cpu栈
char the_cpu[4096] __attribute__((section(".percpu")));
void bp_main(void)
{
  char *w="Hello World!";
  short *p=(short *)0xb8000;
  int i;
  for (i=0;i<12;i++) {
    *p++=w[i]|0x700;
  }
  for(;;);
}
