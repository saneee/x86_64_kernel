#include "stdio_impl.h"

int __lockfile(FILE *f)
{
	return 1;
}

void __unlockfile(FILE *f)
{
}
