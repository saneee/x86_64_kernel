#include <stdio.h>
#include <stdarg.h>
#include "../internal/libc.h"
#undef snprintf
int __snprintf(char *restrict s, size_t n, const char *restrict fmt, ...)
{
	int ret;
	va_list ap;
	va_start(ap, fmt);
	ret = vsnprintf(s, n, fmt, ap);
	va_end(ap);
	return ret;
}
__weak_alias(__snprintf, snprintf);
