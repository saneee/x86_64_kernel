#ifndef _ASM_X86_LINKAGE_H
#define _ASM_X86_LINKAGE_H
#include <yaos/stringify.h>

#undef notrace
#define notrace __attribute__((no_instrument_function))

#ifdef __ASSEMBLY__

#define GLOBAL(name)	\
	.globl name;	\
	name:

#define __ALIGN		.p2align 4, 0x90
#define __ALIGN_STR	__stringify(__ALIGN)

#endif /* __ASSEMBLY__ */

#endif /* _ASM_X86_LINKAGE_H */
