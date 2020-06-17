#ifndef _ASM_X86_TLBFLUSH_H
#define _ASM_X86_TLBFLUSH_H

#include <asm/cpufeature.h>
#include <asm/cpu.h>
/* There are 12 bits of space for ASIDS in CR3 */
#define CR3_HW_ASID_BITS                12

static inline void __native_flush_tlb_one_user(unsigned long addr)
{
    asm volatile("invlpg (%0)" ::"r" (addr) : "memory");

}
       
static inline void flush_tlb_one_page(unsigned long addr)
{
    __native_flush_tlb_one_user(addr);
}
#endif
