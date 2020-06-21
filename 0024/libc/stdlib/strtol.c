#include "../stdio/stdio_impl.h"
#include "../internal/intscan.h"
#include "../internal/shgetc.h"
#include <limits.h>
#include <api/ctype.h>
#include "../internal/libc.h"
#define TOLOWER(x) ((x) | 0x20)
#define isxdigit(c)    (('0' <= (c) && (c) <= '9') || ('a' <= (c) && (c) <= 'f')\
   || ('A' <= (c) && (c) <= 'F'))
#ifndef isdigit
#define isdigit(c)    ('0' <= (c) && (c) <= '9')
#endif
unsigned long strtoul(const char *cp,char **endp, int base)
{
    unsigned long result = 0,value;
 
    if (!base) {
        base = 10;
        if (*cp == '0') {
            base = 8;
            cp++;
            if ((TOLOWER(*cp) == 'x') && isxdigit(cp[1])) {
                cp++;
                base = 16;
            }
        }
    } else if (base == 16) {
        if (cp[0] == '0' && TOLOWER(cp[1]) == 'x')
            cp += 2;
    }
    while (isxdigit(*cp) &&
           (value = isdigit(*cp) ? *cp-'0' : TOLOWER(*cp)-'a'+10) < base) {
        result = result*base + value;
        cp++;
    }
    if (endp)
        *endp = (char *)cp;
    return result;
}
long strtol(const char *cp,char **endp, int base)
{
    if(*cp=='-')
        return -strtoul(cp+1,endp,base);
    return strtoul(cp,endp,base);
}
int  atoi(const     char *nptr)
{
    return  strtol(nptr, (  char  **)NULL, 10);
}


