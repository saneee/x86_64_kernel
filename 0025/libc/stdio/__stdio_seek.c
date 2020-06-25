#include "stdio_impl.h"
#include <yaoscall/api.h>
off_t __stdio_seek(FILE *f, off_t off, int whence)
{
	return sys_lseek(f->fd, off, whence);
}
