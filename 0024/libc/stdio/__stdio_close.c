#include <unistd.h>
#include "stdio_impl.h"
#include <yaoscall/api.h>

int __stdio_close(FILE *f)
{
	return sys_close(f->fd);
}
