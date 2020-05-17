#ifndef __YAOS_SPINLOCK_H
#define __YAOS_SPINLOCK_H
typedef unsigned long spinlock_t;
#include <asm/spinlock.h>
#ifndef ARCH_SPINLOCK
void init_spinlock(__thread spinlock_t *);
void spin_lock(__thread spinlock_t *);
void spin_unlock(__thread spinlock_t *);
#endif
#endif
