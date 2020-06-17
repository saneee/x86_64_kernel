#ifndef _YAOS_BARRIER_H_
#define _YAOS_BARRIER_H_
#include <asm/barrier.h>
#ifndef nop
#define nop()   asm volatile ("nop")
#endif
#ifndef mb
#define mb()    barrier()
#endif
#ifndef rmb
#define rmb()   mb()
#endif
#ifndef wmb
#define wmb()   mb()
#endif

#ifndef dma_rmb
#define dma_rmb()       rmb()
#endif

#ifndef dma_wmb
#define dma_wmb()       wmb()
#endif

#ifndef read_barrier_depends
#define read_barrier_depends()          do { } while (0)
#endif

#ifndef smp_mb
#define smp_mb()        mb()
#endif

#ifndef smp_rmb
#define smp_rmb()       rmb()
#endif

#ifndef smp_wmb
#define smp_wmb()       wmb()
#endif

#ifndef smp_read_barrier_depends
#define smp_read_barrier_depends()      read_barrier_depends()
#endif

#ifndef smp_store_mb
#define smp_store_mb(var, value)  do { WRITE_ONCE(var, value); mb(); } while (0)
#endif

#ifndef smp_mb__before_atomic
#define smp_mb__before_atomic() smp_mb()
#endif

#ifndef smp_mb__after_atomic
#define smp_mb__after_atomic()  smp_mb()
#endif
#define smp_store_release(p, v)                                         \
do {                                                                    \
        compiletime_assert_atomic_type(*p);                             \
        smp_mb();                                                       \
        ACCESS_ONCE(*p) = (v);                                          \
} while (0)

#define smp_load_acquire(p)                                             \
({                                                                      \
        typeof(*p) ___p1 = ACCESS_ONCE(*p);                             \
        compiletime_assert_atomic_type(*p);                             \
        smp_mb();                                                       \
        ___p1;                                                          \
})

#define virt_mb() __smp_mb()
#define virt_mb__before_atomic() __smp_mb__before_atomic()
#define virt_mb__after_atomic()   __smp_mb__after_atomic()
#endif /* _BARRIER_H_ */
