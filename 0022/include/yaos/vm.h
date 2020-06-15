#ifndef _YAOS_VM_H
#define _YAOS_VM_H
extern void *alloc_vm(ulong size);
extern void *alloc_vm_stack(ulong size);
extern void free_vm(ulong addr, ulong size);
extern void free_vm_stack(ulong addr, ulong size);
extern void free_linear(ulong addr, ulong size);
extern void *alloc_linear(ulong size);
#endif
