#include <stdio.h>
void dummy()
{
printf("dummy\n");
}
void __dynlink()
{
dummy();
}

void _Unwind_RaiseException()
{
dummy();
}


void _Unwind_DeleteException()
{
dummy();
}

void _Unwind_SetGR()
{
dummy();
}

void _Unwind_SetIP()
{
dummy();
}


void _Unwind_GetCFA()
{
dummy();
}

