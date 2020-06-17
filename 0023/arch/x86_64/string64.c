/*
 * Most of the string-functions are rather heavily hand-optimized,
 * see especially strsep,strstr,str[c]spn. They should work, but are not
 * very easy to understand. Everything is done entirely within the register
 * set, making the functions fast and clean. String instructions have been
 * used through-out, making for "slightly" unclear code :-)
 *
 * AK: On P4 and K7 using non string instruction implementations might be faster
 * for large memory blocks. But most of them are unlikely to be used on large
 * strings.
 */
#include <yaos/types.h>
#include <yaos/string.h>
#include <yaos/export.h>
#include <yaos/printk.h>

static inline int isdigit(int ch)
{
    return (ch >= '0') && (ch <= '9');
}

int memcmp(const void *s1, const void *s2, size_t len)
{
    u8 diff;

  asm("repe; cmpsb; setnz %0":"=qm"(diff), "+D"(s1), "+S"(s2), "+c"(len));
    return diff;
}

void *memset(void *s, char c, size_t count)
{
    int d0, d1;

    ASSERT(count > 0);
    asm volatile ("rep\n\t" "stosb":"=&c" (d0), "=&D"(d1)
                  :"a"(c), "1"(s), "0"(count)
                  :"memory");

    return s;
}

int strcmp(const char *str1, const char *str2)
{
    const unsigned char *s1 = (const unsigned char *)str1;
    const unsigned char *s2 = (const unsigned char *)str2;
    int delta = 0;

    while (*s1 || *s2) {
        delta = *s1 - *s2;
        if (delta)
            return delta;
        s1++;
        s2++;
    }
    return 0;
}

int strncmp(const char *cs, const char *ct, size_t count)
{
    unsigned char c1, c2;

    while (count) {
        c1 = *cs++;
        c2 = *ct++;
        if (c1 != c2)
            return c1 < c2 ? -1 : 1;
        if (!c1)
            break;
        count--;
    }
    return 0;
}

size_t strnlen(const char *s, size_t maxlen)
{
    const char *es = s;

    while (*es && maxlen) {
        es++;
        maxlen--;
    }

    return (es - s);
}

unsigned int atou(const char *s)
{
    unsigned int i = 0;

    while (isdigit(*s))
        i = i * 10 + (*s++ - '0');
    return i;
}

/* Works only for digits and letters, but small and fast */
#define TOLOWER(x) ((x) | 0x20)

/**
 * strlen - Find the length of a string
 * @s: The string to be sized
 */
size_t strlen(const char *s)
{
    const char *sc;

    for (sc = s; *sc != '\0'; ++sc)
        /* nothing */ ;
    return sc - s;
}

/**
 * strstr - Find the first substring in a %NUL terminated string
 * @s1: The string to be searched
 * @s2: The string to search for
 */
char *strstr(const char *s1, const char *s2)
{
    size_t l1, l2;

    l2 = strlen(s2);
    if (!l2)
        return (char *)s1;
    l1 = strlen(s1);
    while (l1 >= l2) {
        l1--;
        if (!memcmp(s1, s2, l2))
            return (char *)s1;
        s1++;
    }
    return NULL;
}

void *memmove(void *vdst, const void *vsrc, size_t n)
{
    char *dst;
    const char *src;

    dst = vdst;
    src = vsrc;
    while (n-- > 0)
        *dst++ = *src++;
    return vdst;
}
#undef memcpy
void * memcpy(void *to, const void *from, size_t n) {
    return __inline_memcpy(to, from, n);
}
