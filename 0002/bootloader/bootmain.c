#if 1 
void bootmain(void)
{
  short *p = (short *)0xb8000;
  int i;
  for (i=0;i<12;i++) {
    *p++=72|0x700;
  }
  
}
#else
void bootmain(void)
{
  char *w = "Hellow World!"
  short *p = (short *)0xb8000;
  int i;
  for (i=0;i<12;i++) {
    *p++=w[i]|0x700;
  }

}

#endif
