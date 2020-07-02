#ifndef ___CC_H_
#define ___CC_H_
#define __NEED_ptrdiff_t
#define __DEFINED_struct_iovec
#include <yaos/types.h>
#include <yaos/printk.h>
#include <yaos/assert.h>
#include <api/errno.h>
#include <api/stdlib.h>
#define LWIP_PLATFORM_DIAG(x) do {printk x;} while(0)

/** Platform specific assertion handling.\n
 * Note the default implementation pulls in printf, fflush and abort, which may
 * in turn pull in a lot of standard libary code. In resource-constrained
 * systems, this should be defined to something less resource-consuming.
 */
#define LWIP_PLATFORM_ASSERT(x) ASSERT(x)
#define LWIP_NO_STDDEF_H 1
#define LWIP_NO_STDINT_H 1
#define LWIP_NO_CTYPE_H 1
#define LWIP_NO_UNISTD_H 1
#define LWIP_NO_INTTYPES_H 1
#define LWIP_HAVE_INT64 1
typedef signed char s8_t;
typedef signed short s16_t;
typedef signed int s32_t;
typedef signed long s64_t;
typedef uintptr_t mem_ptr_t;
#include <yaos/time.h>
static inline u32_t sys_now(void)
{
    return (u32_t )(hpet_uptime()/1000000UL);
}
static inline u32_t sys_rand(void)
{
    return (u32_t)(hpet_uptime());
}
#define mem_clib_malloc yaos_malloc
#define mem_clib_free yaos_mfree

#define LWIP_RAND  sys_rand

#ifndef PRIx8
#define PRIx8 "x"
#endif
#ifndef PRIu16
#define PRIu16 "u"
#endif
#ifndef PRIx16
#define PRIx16 "x"
#endif
#ifndef PRIu32
#define PRIu32 "u"
#endif
#ifndef PRIx32
#define PRIx32 "x"
#endif
#ifndef PRId16
#define PRId16 "d"
#endif
#ifndef PRIuPTR
#define PRIuPTR "u"
#endif
#ifndef PRId32
#define PRId32 "d"
#endif
#ifndef X8_F
#define X8_F  "02" PRIx8
#endif
#ifndef U16_F
#define U16_F PRIu16
#endif
#ifndef S16_F
#define S16_F PRId16
#endif
#ifndef X16_F
#define X16_F PRIx16
#endif
#ifndef U32_F
#define U32_F PRIu32
#endif
#ifndef S32_F
#define S32_F PRId32
#endif
#ifndef X32_F
#define X32_F PRIx32
#endif
#ifndef SZT_F
#define SZT_F PRIuPTR
#endif

#endif
