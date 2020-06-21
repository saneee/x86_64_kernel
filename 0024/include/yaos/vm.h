#ifndef _YAOS_VM_H
#define _YAOS_VM_H
typedef enum vm_zone {
  VM_ZONE_VMALLOC=1,
  VM_ZONE_KERNEL_STACK=2,
  VM_ZONE_LOW_MMAP=3,//low address ,bit 63 is zero
} vm_zone_t;

extern void *alloc_vm(vm_zone_t,ulong size);
extern void *alloc_vm_stack(ulong size);
extern void free_vm(vm_zone_t,ulong addr, ulong size);
extern void free_vm_stack(ulong addr, ulong size);
extern void free_linear(vm_zone_t,ulong addr, ulong size);
extern void *alloc_linear(vm_zone_t,ulong size);
#endif
