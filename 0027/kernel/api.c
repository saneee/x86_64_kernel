#include <yaos/yaoscall.h>
#include <yaos/kheap.h>
#include <asm/pgtable.h>
#include <yaos/rett.h>
#include <yaos/kernel.h>
#include <yaoscall/page.h>
#include <yaos/assert.h>
#include <yaos/vm.h>
#include <asm/phymem.h>
#include <asm/pgtable.h>
#include <yaos/printk.h>
#include <yaos/errno.h>
static ssize_t api_readv(int fd, struct iovec *pvec, int iovcnt)
{
    printk("try read fd:%d\n",fd);
    return -1;
}
static ssize_t api_writev(int fd, struct iovec *pvec, int iovcnt)
{
    ssize_t vga_write(void *p, size_t s); 
    if (fd == 1|| fd==2) {
        return vga_write(pvec[1].iov_base,pvec[1].iov_len);
    }
    printk("try write fd:%d\n",fd);
    return -1;
}
static off_t api_lseek(int fd, off_t off, int whence) 
{
    return off;
}

DECLARE_YAOSCALL(YAOS_readv, api_readv);
DECLARE_YAOSCALL(YAOS_writev,api_writev);
DECLARE_YAOSCALL(YAOS_lseek,api_lseek);
