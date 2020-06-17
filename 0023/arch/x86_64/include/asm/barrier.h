#ifndef _ASM_BARRIER_H
#define _ASM_BARRIER_H
#define mb()    asm volatile("mfence":::"memory")
#define rmb()   asm volatile("lfence":::"memory")
#define wmb()   asm volatile("sfence" ::: "memory")

#define dma_rmb()       barrier()
#define dma_wmb()       barrier()
#define __smp_mb()      asm volatile("lock; addl $0,-4(%%rsp)" ::: "memory", "cc")
#define __smp_rmb()     dma_rmb()
#define __smp_wmb()     barrier()
#define __smp_store_mb(var, value) do { (void)xchg(&var, value); } while (0)

#define __smp_store_release(p, v)                                       \
do {                                                                    \
        compiletime_assert_atomic_type(*p);                             \
        barrier();                                                      \
        WRITE_ONCE(*p, v);                                              \
} while (0)

#define __smp_load_acquire(p)                                           \
({                                                                      \
        typeof(*p) ___p1 = READ_ONCE(*p);                               \
        compiletime_assert_atomic_type(*p);                             \
        barrier();                                                      \
        ___p1;                                                          \
})

/* Atomic operations are already serializing on x86 */
#define __smp_mb__before_atomic()       do { } while (0)
#define __smp_mb__after_atomic()        do { } while (0)

#endif
