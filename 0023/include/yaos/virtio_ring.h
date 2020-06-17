#ifndef _YAOS_VIRTIO_RING_H
#define _YAOS_VIRTIO_RING_H
#include <yaos/barrier.h>
#include <asm/cmpxchg.h>
#define virt_mb() __smp_mb()
#define virt_rmb() __smp_rmb()
#define virt_wmb() __smp_wmb()
#define virt_read_barrier_depends() __smp_read_barrier_depends()
#define virt_store_mb(var, value) __smp_store_mb(var, value)
#define virt_mb__before_atomic() __smp_mb__before_atomic()
#define virt_mb__after_atomic() __smp_mb__after_atomic()
#define virt_store_release(p, v) __smp_store_release(p, v)
#define virt_load_acquire(p) __smp_load_acquire(p)

typedef u16_t __virtio16;
static inline void virtio_mb(bool weak_barriers)
{
        if (weak_barriers)
                virt_mb();
        else
                mb();
}

static inline void virtio_rmb(bool weak_barriers)
{
        if (weak_barriers)
                virt_rmb();
        else
                dma_rmb();
}

static inline void virtio_wmb(bool weak_barriers)
{
        if (weak_barriers)
                virt_wmb();
        else
                dma_wmb();
}
static inline void virtio_store_mb(bool weak_barriers,
                                   __virtio16 *p, __virtio16 v)
{
        if (weak_barriers) {
                virt_store_mb(*p, v);
        } else {
                WRITE_ONCE(*p, v);
                mb();
        }
}

struct virtio_device;

#endif
