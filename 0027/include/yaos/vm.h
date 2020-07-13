#ifndef _YAOS_VM_H
#define _YAOS_VM_H
#include <asm/pgtable.h>
typedef enum vm_zone {
  VM_ZONE_VMALLOC=1,
  VM_ZONE_KERNEL_STACK=2,
  VM_ZONE_LOW_MMAP=3,//low address ,bit 63 is zero
  VM_ZONE_KERNEL_BELOW1G=4, //KERNEL_BASE-1G
} vm_zone_t;

extern void *alloc_vm(vm_zone_t,ulong size);
extern void *alloc_vm_stack(ulong size);
extern void free_vm(vm_zone_t,ulong addr, ulong size);
extern void free_vm_stack(ulong addr, ulong size);
extern void free_linear(vm_zone_t,ulong addr, ulong size);
extern void *alloc_linear_impl(vm_zone_t,void *addr, ulong size);
static inline void *alloc_linear_at(vm_zone_t z,void *addr, ulong size)
{
    return alloc_linear_impl(z, addr, size);
}
static inline void *alloc_linear(vm_zone_t z, ulong size)
{
    return alloc_linear_impl(z, NULL, size);
}
extern void *alloc_linear_hint(void *addr, ulong size);
extern void free_linear_hint(void *addr, ulong size);

static inline int get_vm_zone_from_addr(void *p) 
{
    ulong addr = (ulong)p;
    if (addr>=VMALLOC_START && addr<= VMALLOC_END) return VM_ZONE_VMALLOC;
    if (addr>=LOW_MMAP_START && addr<=LOW_MMAP_END) return VM_ZONE_LOW_MMAP;
    if (addr>=KERNEL_BELOW_1G_START ) return VM_ZONE_KERNEL_BELOW1G;
    return 0;

}
void register_stack(ulong start, ulong end, const char *name,u32 cpu);
void find_stack(ulong addr);
void dump_stack();
bool check_stack(ulong addr);
#endif
