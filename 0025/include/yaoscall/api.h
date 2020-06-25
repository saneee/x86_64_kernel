
#ifndef _SYSCALL_API_H
#define _SYSCALL_API_H
#include <types.h>
#include <yaos/yaoscall.h>
#include <yaos/rett.h>
static inline ssize_t writev(int fd, struct iovec *p, int iovcnt)
{
    ssize_t (*pfunc)(int fd, struct iovec *p, int iovcnt)=yaoscall_p(YAOS_writev);
    return (*pfunc)(fd, p, iovcnt);
}
static inline ssize_t readv(int fd, struct iovec *p, int iovcnt)
{
    ssize_t (*pfunc)(int fd, struct iovec *p, int iovcnt)=yaoscall_p(YAOS_readv);
    return (*pfunc)(fd, p, iovcnt);
}
static inline off_t sys_lseek(int fd, off_t off, int wh)
{
    off_t (*pfunc)(int fd, off_t off, int w)=yaoscall_p(YAOS_lseek);
    return (*pfunc)(fd, off, wh);

}
static inline int sys_close(int fd)
{
    return 0;
}
#endif
