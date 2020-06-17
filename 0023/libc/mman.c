#define _GNU_SOURCE
#include <vargs.h>
#include <yaoscall/page.h>
#include <api/sys/mman.h>
#include <yaos/errno.h>
#include "./internal/libc.h"
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
    printf("mmap:%lx,size:%lx,prot:%x,flags:%x,fd:%x,offset:%x\n",addr,length,prot,flags,fd,offset);
    //always alloc 
    addr = yaos_alloc_linear_low_mmap(length);
    if (!addr)return MAP_FAILED;
   
    ulong mmflags = map_flags_base() 
             | ((prot & PROT_WRITE)?map_flags_write():0)
             | ((prot & PROT_EXEC)?map_flags_code():0);
    ret_t ret = yaos_alloc_at((ulong)addr,length,mmflags);
    if (ret.e != OK) return MAP_FAILED;
    return addr;
    
    
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
    printf("munmap:%lx,size:%lx\n",addr,length);
    ret_t ret = yaos_vm_free_low_mmap((ulong)addr,length);
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
    printf("mremap:old:%lx,old_size:%lx,new_len:%lx,flags:%x\n",old_addr,old_len,new_len,flags);

    va_list ap;
    void *new_addr;

    va_start(ap, flags);
    new_addr = va_arg(ap, void *);
    va_end(ap);
    if (!old_len)old_len = new_len;
    if (!(flags & MREMAP_FIXED) || !new_addr) {
        new_addr = yaos_alloc_linear_low_mmap(new_len);
        if (!new_addr)return MAP_FAILED;

    }
    if (new_len >old_len) {
        ulong mmflags = yaos_map_flags_at((ulong)old_addr); 
        ret_t ret = yaos_alloc_at((ulong)new_addr+old_len, new_len - old_len, mmflags);
        if (ret.e != OK) return MAP_FAILED;
    }
    else if (old_len >new_len && !(flags & MREMAP_DONTUNMAP)) {
        yaos_vm_free_low_mmap((ulong)new_addr+new_len, old_len - new_len);
    }
    if (yaos_map_copy((ulong)new_addr, (ulong)old_addr, old_len) != OK) return MAP_FAILED;
    if (!(flags & MREMAP_DONTUNMAP)) {
        yaos_vm_free_low_mmap((ulong)old_addr, old_len);
    }
    return new_addr;
}

weak_alias(__mremap, mremap);

