#ifndef _DRIVERS_MMIO_H
#define _DRIVERS_MMIO_H

#include <yaos/types.h>
#include <asm/pgtable.h>

static inline void mmio_setb(u64 addr, u8 val)
{
    *(volatile u8 *)(addr) = val;
}

static inline void mmio_setw(u64 addr, u16 val)
{
    *(volatile u16 *)(addr) = val;
}

static inline void mmio_setl(u64 addr, u32 val)
{
    *(volatile u32 *)(addr) = val;
}

static inline void mmio_setq(u64 addr, u64 val)
{
    *(volatile u64 *)(addr) = val;
}

static inline u8 mmio_getb(u64 addr)
{
    return (*(volatile u8 *)(addr));
}

static inline u16 mmio_getw(u64 addr)
{
    return (*(volatile u16 *)(addr));
}

static inline u32 mmio_getl(u64 addr)
{
    return (*(volatile u32 *)(addr));
}

static inline u64 mmio_getq(u64 addr)
{
    return (*(volatile u64 *)(addr));
}

static inline ulong mmio_map(u64 paddr, size_t size_bytes)
{
    return (ulong) ioremap_nocache(paddr, size_bytes);
}

static inline void mmio_unmap(u64 addr, size_t size_bytes)
{
    // FIXME: implement
}
#endif
