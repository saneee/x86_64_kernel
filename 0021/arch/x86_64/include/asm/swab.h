#ifndef _ASM_X86_SWAB_H
#define _ASM_X86_SWAB_H
#include <yaos/types.h>
#include <yaos/compiler.h>
static inline __attribute_const__ __u32 __arch_swab32(__u32 val)
{
        asm("bswapl %0" : "=r" (val) : "0" (val));
        return val;
}
#define __arch_swab32 __arch_swab32

static inline __attribute_const__ __u64 __arch_swab64(__u64 val)
{
        asm("bswapq %0" : "=r" (val) : "0" (val));
        return val;
}
#define __arch_swab64 __arch_swab64
#endif
