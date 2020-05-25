/*
 * Copyright (C) 2015 Yiwu Saneee Network CO, Ltd.
 *
 * This work is open source software, licensed under the terms of the
 * BSD license as described in the LICENSE file in the top-level directory.
 */

#ifndef _YAOS_TYPES_H_
#define _YAOS_TYPES_H_ 1
#include <yaos/rett.h>
typedef _Bool 	bool;
typedef char 	int8_t;
typedef short 	int16_t;
typedef int	int32_t;
typedef long 	int64_t;

typedef  unsigned char uint8_t;
typedef  unsigned short uint16_t;
typedef  unsigned int uint32_t;
typedef  unsigned long uint64_t;
typedef unsigned long phys_bytes;
typedef unsigned long off_t;
typedef unsigned long uintptr_t;
typedef unsigned long u_long;
typedef unsigned int u_int;
typedef unsigned short u_short;
typedef unsigned char u_char;



typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

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
/* this is a special 64bit data type that is 8-byte aligned */
#define aligned_u64 __u64 __attribute__((aligned(8)))
typedef struct {
    long counter;
} atomic_t;

typedef unsigned gfp_t;
typedef u64 phys_addr_t;

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
   
#include <yaos/stddef.h>
#endif /* _YAOS_TYPES_H_ */
