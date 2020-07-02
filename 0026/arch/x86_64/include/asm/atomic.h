#ifndef _ASM_ATOMIC_H
#define _ASM_ATOMIC_H
#include <asm/alternative.h>
#include <asm/cmpxchg.h>

#define ATOMIC_INIT(i)        { (i) }
static inline long atomic_read(const atomic_t * v)
{
    return ACCESS_ONCE((v)->counter);
}

static inline void atomic_set(atomic_t * v, long i)
{
    v->counter = i;
}

static __always_inline void atomic_add(long i, atomic_t * v)
{
    asm volatile (LOCK_PREFIX "addq %1,%0":"=m"(v->counter)
                  :"er"(i), "m"(v->counter));
}

static inline void atomic_sub(long i, atomic_t * v)
{
    asm volatile (LOCK_PREFIX "subq %1,%0":"=m"(v->counter)
                  :"er"(i), "m"(v->counter));
}

static inline int atomic_sub_and_test(long i, atomic_t * v)
{
    GEN_BINARY_RMWcc(LOCK_PREFIX "subq", v->counter, "er", i, "%0", "e");
}

static __always_inline void atomic_inc(atomic_t * v)
{
    asm volatile (LOCK_PREFIX "incq %0":"=m"(v->counter)
                  :"m"(v->counter));
}

static __always_inline void atomic_dec(atomic_t * v)
{
    asm volatile (LOCK_PREFIX "decq %0":"=m"(v->counter)
                  :"m"(v->counter));
}

static inline int atomic_dec_and_test(atomic_t * v)
{
    GEN_UNARY_RMWcc(LOCK_PREFIX "decq", v->counter, "%0", "e");
}

static inline int atomic_inc_and_test(atomic_t * v)
{
    GEN_UNARY_RMWcc(LOCK_PREFIX "incq", v->counter, "%0", "e");
}

static inline int atomic_add_negative(long i, atomic_t * v)
{
    GEN_BINARY_RMWcc(LOCK_PREFIX "addq", v->counter, "er", i, "%0", "s");
}

static __always_inline long atomic_add_return(long i, atomic_t * v)
{
    return i + xadd(&v->counter, i);
}

static inline long atomic_sub_return(long i, atomic_t * v)
{
    return atomic_add_return(-i, v);
}

#define atomic_inc_return(v)  (atomic_add_return(1, (v)))
#define atomic_dec_return(v)  (atomic_sub_return(1, (v)))

static inline long atomic_cmpxchg(atomic_t * v, long old, long new)
{
    return cmpxchg(&v->counter, old, new);
}

static inline long atomic_xchg(atomic_t * v, long new)
{
    return xchg(&v->counter, new);
}

static inline int atomic_add_unless(atomic_t * v, long a, long u)
{
    long c, old;

    c = atomic_read(v);
    for (;;) {
        if (unlikely(c == (u)))
            break;
        old = atomic_cmpxchg((v), c, c + (a));
        if (likely(old == c))
            break;
        c = old;
    }
    return c != (u);
}

#define atomic_inc_not_zero(v) atomic_add_unless((v), 1, 0)

static inline long atomic_dec_if_positive(atomic_t * v)
{
    long c, old, dec;

    c = atomic_read(v);
    for (;;) {
        dec = c - 1;
        if (unlikely(dec < 0))
            break;
        old = atomic_cmpxchg((v), c, dec);
        if (likely(old == c))
            break;
        c = old;
    }
    return dec;
}

#endif
