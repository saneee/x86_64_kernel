#include <errno.h>

const char *const sys_errlist[] = {
#define E(num, message) [ num] = message,
#include "./__strerror.h"
#undef E
};

const int sys_nerr = sizeof(sys_errlist) / sizeof(sys_errlist[0]);
const char *strerror(int e)
{
    return sys_errlist[e<sys_nerr && e>=0?e:0];
}

