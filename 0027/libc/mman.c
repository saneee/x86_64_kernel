#define _GNU_SOURCE
#include <vargs.h>
#include <yaoscall/page.h>
#include <api/sys/mman.h>
#include <yaos/errno.h>
#include "./internal/libc.h"
#include <yaos/printk.h>
#if  0
extern void show_call_stack(int i);
#define DEBUG_PRINT printk
#define DEBUG_CALLSTACK() show_call_stack(8)
#else
#define DEBUG_PRINT inline_printk
#define DEBUG_CALLSTACK()
#endif

int mprotect(void *addr, size_t len, int prot)
{
    return 0;
}
int mmap_validate(void *addr, size_t length, int flags, off_t offset)
{
    return 0;
}
void *mmap(void *addr, size_t length, int prot, int flags,
           int fd, off_t offset)
{
    DEBUG_PRINT("mmap:%lx,size:%lx,prot:%x,flags:%x,fd:%x,offset:%x\n",addr,length,prot,flags,fd,offset);
    //always alloc 
    addr = addr==NULL? yaos_alloc_linear_low_mmap(addr, length): yaos_alloc_linear_hint(addr, length);
    DEBUG_PRINT("mmap:add:%lx\n", addr);
    DEBUG_CALLSTACK();
    if (!addr) goto map_fail;
   
    ulong mmflags = map_flags_base() 
             | ((prot & PROT_WRITE)?map_flags_write():0)
             | ((prot & PROT_EXEC)?map_flags_code():0);
    ret_t ret = yaos_alloc_at((ulong)addr,length,mmflags);
    if (ret.e != OK) goto map_fail;
    return addr;
map_fail:
    DEBUG_PRINT("mmap failed.........\n");
    return MAP_FAILED;    
    
}
void *mmap64(void *addr, size_t length, int prot, int flags,
                      int fd, off64_t offset)
    __attribute__((alias("mmap")));
int munmap_validate(void *addr, size_t length)
{
    return 0;
}
int munmap(void *addr, size_t length)
{
    DEBUG_PRINT("munmap:%lx,size:%lx\n",addr,length);
    ret_t ret = yaos_vm_free_hint(addr,length);
    DEBUG_PRINT("yaos_vm_free_hint ret:%lx\n",ret.e);
    if(ret.e != OK) return -1;
    return 0;
}
int msync(void *addr, size_t length, int flags)
{
    return 0;
}
int mincore(void *addr, size_t length, unsigned char *vec)
{
    return -1;
}
int madvise(void *addr, size_t length, int advice)
{
    return -1;
}
void *__mremap(void *old_addr, size_t old_len, size_t new_len, int flags, ...)
{
    DEBUG_PRINT("mremap:old:%lx,old_size:%lx,new_len:%lx,flags:%x\n",old_addr,old_len,new_len,flags);


    va_list ap;
    void *new_addr;

    va_start(ap, flags);
    new_addr = va_arg(ap, void *);
    va_end(ap);
    if (!old_len)old_len = new_len;
    if (!(flags & MREMAP_FIXED) || !new_addr) {
        ulong alloc_size = new_len + ((ulong)old_addr&(MIN_VM_ALLOC_SIZE-1));
        new_addr = yaos_alloc_linear_low_mmap(NULL, alloc_size);
        if (!new_addr) goto mapfail;
        new_addr = (void *)((ulong)new_addr + ((ulong)old_addr&(MIN_VM_ALLOC_SIZE-1)));
    }
    if (new_len >old_len) {
        ulong mmflags = yaos_map_flags_at((ulong)old_addr); 
printk("alloc at :%lx,%lx,%lx\n",(ulong)new_addr+old_len, new_len - old_len, mmflags);
        ret_t ret = yaos_alloc_at((ulong)new_addr+old_len, new_len - old_len, mmflags);
        if (ret.e != OK)  goto mapfail;
    }
    else if (old_len >new_len  && old_len-new_len>=MIN_VM_ALLOC_SIZE && !(flags & MREMAP_DONTUNMAP)) {
        yaos_vm_unmap_low_mmap((ulong)new_addr+new_len, old_len - new_len);
    }
printk("map_copy newaddr:%lx, oldaddr:%lx, size:%lx\n",new_addr, old_addr, old_len);
    if (yaos_map_copy((ulong)new_addr, (ulong)old_addr, old_len) != OK) goto mapfail;
    if (!(flags & MREMAP_DONTUNMAP)) {
        yaos_vm_unmap_low_mmap((ulong)old_addr, old_len);
    }
    return new_addr;
mapfail:
    DEBUG_PRINT("mremap failed ............\n");
    return MAP_FAILED;
}

weak_alias(__mremap, mremap);

