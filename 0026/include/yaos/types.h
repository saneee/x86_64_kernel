/*
 * Copyright (C) 2015 Yiwu Saneee Network CO, Ltd.
 *
 * This work is open source software, licensed under the terms of the
 * BSD license as described in the LICENSE file in the top-level directory.
 */

#ifndef _YAOS_TYPES_H_
#define _YAOS_TYPES_H_ 1
#include <yaos/rett.h>
#ifndef LINUX
typedef _Bool 	bool;
#if !defined(__DEFINED_int8_t)
typedef signed char int8_t;
#define __DEFINED_int8_t
#endif
#if !defined(__DEFINED_int16_t)
typedef signed short int16_t;
#define __DEFINED_int16_t
#endif

#if !defined(__DEFINED_int32_t)
typedef signed int int32_t;
#define __DEFINED_int32_t
#endif

#if !defined(__DEFINED_int64_t)
typedef signed long int64_t;
#define __DEFINED_int64_t
#endif


typedef  unsigned char uint8_t;
typedef  unsigned short uint16_t;
typedef  unsigned int uint32_t;
typedef  unsigned long uint64_t;
typedef unsigned long phys_bytes;
#ifndef __DEFINED_off_t
typedef long off_t;
#define __DEFINED_off_t
#endif
#ifndef __DEFINED_time_t
typedef long time_t;
#define __DEFINED_time_t
#endif

typedef unsigned long off64_t;
typedef unsigned long uintptr_t;
typedef long intptr_t;
typedef unsigned long u_long;
typedef unsigned int u_int;
typedef unsigned short u_short;
typedef unsigned char u_char;




typedef unsigned long ulong;
typedef unsigned int  uint;
typedef unsigned char uchar;

typedef  long n_int;
typedef  unsigned long n_uint;
typedef unsigned char uchar;
typedef  unsigned char __u8;
typedef  unsigned short __u16;
typedef  unsigned int __u32;
typedef  unsigned long __u64;
typedef char __s8;
typedef short __s16;
typedef int __s32;
typedef long __s64;
#ifndef __kernel_size_t
typedef __u64  __kernel_size_t;
typedef __s64  __kernel_ssize_t;
typedef __u64  __kernel_loff_t;
#endif 
typedef __kernel_size_t size_t;
typedef __kernel_ssize_t ssize_t;
typedef __kernel_loff_t  loff_t;
typedef unsigned long time64_t;

typedef __u32 uint32;
typedef __u64 uint64;
typedef __u16 ushort;
typedef __u64 uintp;
typedef uintp pde_t;
typedef unsigned int mode_t;
#ifdef __CHECKER__
#define __bitwise__ __attribute__((bitwise))
#else
#define __bitwise__
#endif
#define __bitwise __bitwise__

typedef __u16 __bitwise __le16;
typedef __u16 __bitwise __be16;
typedef __u32 __bitwise __le32;
typedef __u32 __bitwise __be32;
typedef __u64 __bitwise __le64;
typedef __u64 __bitwise __be64;

typedef __u16 __bitwise __sum16;
typedef __u32 __bitwise __wsum;

/* this is a special 64bit data type that is 8-byte aligned */
#define aligned_u64 __u64 __attribute__((aligned(8)))
typedef struct {
    long counter;
} atomic_t;

typedef unsigned gfp_t;
struct list_head {
    struct list_head *next, *prev;
};

struct hlist_head {
    struct hlist_node *first;
};
struct hlist_node {
    struct hlist_node *next, **pprev;
};
struct callback_head {
    struct callback_head *next;
    void (*func) (struct callback_head * head);
};
#if !defined(__DEFINED_struct_iovec)
struct iovec {
    void *iov_base; size_t iov_len; 
};  
#define __DEFINED_struct_iovec

#endif 
#include <yaos/stddef.h>
#else /* LINUX */
#include <linux/types.h>
#include <sys/types.h>
#endif

typedef  unsigned char uint8_t;
typedef  unsigned short uint16_t;
typedef  unsigned int uint32_t;
typedef  unsigned long uint64_t;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef u8 u8_t;
typedef u16 u16_t;
typedef u32 u32_t;
typedef u64 u64_t;
typedef u64 phys_addr_t;
typedef u64 dma_addr_t;

#endif /* _YAOS_TYPES_H_ */
