#ifndef __YAOS_PRINTK__H
#define __YAOS_PRINTK__H 1
int sprintf(char *buf, const char *fmt, ...);
int printf(const char *fmt, ...);
int printk(const char *fmt, ...);
void panic(const char *fmt, ...);
static inline void inline_printk(const char *fmt,...){};
void dump_mem(void *p, unsigned int len);
#endif
