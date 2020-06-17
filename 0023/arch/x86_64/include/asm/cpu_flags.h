#ifndef _ASM_CPU_FLAGS_H
#define _ASM_CPU_FLAGS_H
#define X86_EFLAGS_CF_BIT       0	/* Carry Flag */
#define X86_EFLAGS_CF           _BITUL(X86_EFLAGS_CF_BIT)
#define X86_EFLAGS_FIXED_BIT    1	/* Bit 1 - always on */
#define X86_EFLAGS_FIXED        _BITUL(X86_EFLAGS_FIXED_BIT)
#define X86_EFLAGS_PF_BIT       2	/* Parity Flag */
#define X86_EFLAGS_PF           _BITUL(X86_EFLAGS_PF_BIT)
#define X86_EFLAGS_AF_BIT       4	/* Auxiliary carry Flag */
#define X86_EFLAGS_AF           _BITUL(X86_EFLAGS_AF_BIT)
#define X86_EFLAGS_ZF_BIT       6	/* Zero Flag */
#define X86_EFLAGS_ZF           _BITUL(X86_EFLAGS_ZF_BIT)
#define X86_EFLAGS_SF_BIT       7	/* Sign Flag */
#define X86_EFLAGS_SF           _BITUL(X86_EFLAGS_SF_BIT)
#define X86_EFLAGS_TF_BIT       8	/* Trap Flag */
#define X86_EFLAGS_TF           _BITUL(X86_EFLAGS_TF_BIT)
#define X86_EFLAGS_IF_BIT       9	/* Interrupt Flag */
#define X86_EFLAGS_IF           _BITUL(X86_EFLAGS_IF_BIT)
#define X86_EFLAGS_DF_BIT       10	/* Direction Flag */
#define X86_EFLAGS_DF           _BITUL(X86_EFLAGS_DF_BIT)
#define X86_EFLAGS_OF_BIT       11	/* Overflow Flag */
#define X86_EFLAGS_OF           _BITUL(X86_EFLAGS_OF_BIT)
#define X86_EFLAGS_IOPL_BIT     12	/* I/O Privilege Level (2 bits) */
#define X86_EFLAGS_IOPL         (_AC(3,UL) << X86_EFLAGS_IOPL_BIT)
#define X86_EFLAGS_NT_BIT       14	/* Nested Task */
#define X86_EFLAGS_NT           _BITUL(X86_EFLAGS_NT_BIT)
#define X86_EFLAGS_RF_BIT       16	/* Resume Flag */
#define X86_EFLAGS_RF           _BITUL(X86_EFLAGS_RF_BIT)
#define X86_EFLAGS_VM_BIT       17	/* Virtual Mode */
#define X86_EFLAGS_VM           _BITUL(X86_EFLAGS_VM_BIT)
#define X86_EFLAGS_AC_BIT       18	/* Alignment Check/Access Control */
#define X86_EFLAGS_AC           _BITUL(X86_EFLAGS_AC_BIT)
#define X86_EFLAGS_AC_BIT       18	/* Alignment Check/Access Control */
#define X86_EFLAGS_AC           _BITUL(X86_EFLAGS_AC_BIT)
#define X86_EFLAGS_VIF_BIT      19	/* Virtual Interrupt Flag */
#define X86_EFLAGS_VIF          _BITUL(X86_EFLAGS_VIF_BIT)
#define X86_EFLAGS_VIP_BIT      20	/* Virtual Interrupt Pending */
#define X86_EFLAGS_VIP          _BITUL(X86_EFLAGS_VIP_BIT)
#define X86_EFLAGS_ID_BIT       21	/* CPUID detection */
#define X86_EFLAGS_ID           _BITUL(X86_EFLAGS_ID_BIT)

#define X86_CR0_PE_BIT          0	/* Protection Enable */
#define X86_CR0_PE              _BITUL(X86_CR0_PE_BIT)
#define X86_CR0_MP_BIT          1	/* Monitor Coprocessor */
#define X86_CR0_MP              _BITUL(X86_CR0_MP_BIT)
#define X86_CR0_EM_BIT          2	/* Emulation */
#define X86_CR0_EM              _BITUL(X86_CR0_EM_BIT)
#define X86_CR0_TS_BIT          3	/* Task Switched */
#define X86_CR0_TS              _BITUL(X86_CR0_TS_BIT)
#define X86_CR0_ET_BIT          4	/* Extension Type */
#define X86_CR0_ET              _BITUL(X86_CR0_ET_BIT)
#define X86_CR0_NE_BIT          5	/* Numeric Error */
#define X86_CR0_NE              _BITUL(X86_CR0_NE_BIT)
#define X86_CR0_WP_BIT          16	/* Write Protect */
#define X86_CR0_WP              _BITUL(X86_CR0_WP_BIT)
#define X86_CR0_AM_BIT          18	/* Alignment Mask */
#define X86_CR0_AM              _BITUL(X86_CR0_AM_BIT)
#define X86_CR0_NW_BIT          29	/* Not Write-through */
#define X86_CR0_NW              _BITUL(X86_CR0_NW_BIT)
#define X86_CR0_CD_BIT          30	/* Cache Disable */
#define X86_CR0_CD              _BITUL(X86_CR0_CD_BIT)
#define X86_CR0_PG_BIT          31	/* Paging */
#define X86_CR0_PG              _BITUL(X86_CR0_PG_BIT)

/*
 * Paging options in CR3
 */
#define X86_CR3_PWT_BIT         3	/* Page Write Through */
#define X86_CR3_PWT             _BITUL(X86_CR3_PWT_BIT)
#define X86_CR3_PCD_BIT         4	/* Page Cache Disable */
#define X86_CR3_PCD             _BITUL(X86_CR3_PCD_BIT)
#define X86_CR3_PCID_MASK       _AC(0x00000fff,UL)	/* PCID Mask */

#ifdef __ASSEMBLY__
#define _AC(X,Y)        X
#define _AT(T,X)        X
#else
#define __AC(X,Y)       (X##Y)
#define _AC(X,Y)        __AC(X,Y)
#define _AT(T,X)        ((T)(X))
#endif

#define _BITUL(x)       (_AC(1,UL) << (x))
#define _BITULL(x)      (_AC(1,ULL) << (x))

#endif
