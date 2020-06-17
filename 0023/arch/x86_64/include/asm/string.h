#ifndef _ASM_STRING_H
#define _ASM_STRING_H
#include <yaos/compiler.h>
#include <yaos/assert.h>

/* Written 2002 by Andi Kleen */

/* Only used for special circumstances. Stolen from i386/string.h */
static __always_inline void *__inline_memcpy(void *to, const void *from,
	     size_t n)
{
    unsigned long d0, d1, d2;
    asm volatile ("rep ; movsl\n\t"
                  "testb $2,%b4\n\t"
                  "je 1f\n\t"
                  "movsw\n"
                  "1:\ttestb $1,%b4\n\t"
                  "je 2f\n\t" "movsb\n" "2:":"=&c" (d0), "=&D"(d1), "=&S"(d2)
                  :"0"(n / 4), "q"(n), "1"((long)to), "2"((long)from)
                  :"memory");

    return to;
}

/* Even with __builtin_ the compiler may decide to use the out of line
   function. */

#define __HAVE_ARCH_MEMCPY 1
extern void *__memcpy(void *to, const void *from, size_t len);

#ifndef CONFIG_KMEMCHECK
#if (__GNUC__ == 4 && __GNUC_MINOR__ >= 3) || __GNUC__ > 4
extern void *memcpy(void *to, const void *from, size_t len);
#else
#define memcpy(dst, src, len)					\
({								\
	size_t __len = (len);					\
	void *__ret;						\
	if (__builtin_constant_p(len) && __len >= 64)		\
		__ret = __memcpy((dst), (src), __len);		\
	else							\
		__ret = __builtin_memcpy((dst), (src), __len);	\
	__ret;							\
})
#endif
#else
/*
 * kmemcheck becomes very happy if we use the REP instructions unconditionally,
 * because it means that we know both memory operands in advance.
 */
#endif
#define memcpy(dst, src, len) __inline_memcpy((dst), (src), (len))

#define __HAVE_ARCH_MEMSET
void *__memset(void *s, int c, size_t n);
void *memset(void *s, char c, size_t count);

#define __HAVE_ARCH_MEMMOVE
void *memmove(void *dest, const void *src, size_t count);
void *__memmove(void *dest, const void *src, size_t count);

int memcmp(const void *cs, const void *ct, size_t count);
size_t strlen(const char *s);
char *strcpy(char *dest, const char *src);
char *strcat(char *dest, const char *src);
int strcmp(const char *cs, const char *ct);

#define __HAVE_ARCH_STRCPY
extern char *strcpy(char *dest, const char *src);

#define __HAVE_ARCH_STRNCPY
extern char *strncpy(char *dest, const char *src, size_t count);

#define __HAVE_ARCH_STRCAT
extern char *strcat(char *dest, const char *src);

#define __HAVE_ARCH_STRNCAT
extern char *strncat(char *dest, const char *src, size_t count);

#define __HAVE_ARCH_STRCMP
extern int strcmp(const char *cs, const char *ct);

#define __HAVE_ARCH_STRNCMP
extern int strncmp(const char *cs, const char *ct, size_t count);

#define __HAVE_ARCH_STRCHR
extern char *strchr(const char *s, int c);

#define __HAVE_ARCH_STRLEN
extern size_t strlen(const char *s);

#define __HAVE_ARCH_STRNLEN
extern size_t strnlen(const char *s, size_t count);

#if defined(CONFIG_KASAN) && !defined(__SANITIZE_ADDRESS__)

/*
 * For files that not instrumented (e.g. mm/slub.c) we
 * should use not instrumented version of mem* functions.
 */

#undef memcpy
#define memcpy(dst, src, len) __memcpy(dst, src, len)
#define memmove(dst, src, len) __memmove(dst, src, len)
#define memset(s, c, n) __memset(s, c, n)
#endif

#endif
