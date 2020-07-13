/*
 * Copyright (C) 2015 Yiwu Saneee Network CO, Ltd.
 *
 * This work is open source software, licensed under the terms of the
 * BSD license as described in the LICENSE file in the top-level directory.
 */

#ifndef _YAOS_ASSERT_H_
#define _YAOS_ASSERT_H_ 1

#ifndef __
#ifdef DEBUG
#ifndef BUG
#define BUG() do {void panic(const char *fmt, ...);panic("bug");} while (1)
#endif
extern int printk(const char *,...);
#define ASSERT(x)                                                       \
do {                                                                    \
        if (!(x)) {                                                     \
                printk("assertion failed %s: %d: %s\n",      \
                       __FILE__, __LINE__, #x);                         \
                BUG();                                                  \
        }                                                               \
} while (0)
#define ASSERT_PRINT(x,fmt,arg...) \
do {                                                                    \
        if (!(x)) {                                                     \
                printk("assertion failed %s: %d: %s\n",      \
                       __FILE__, __LINE__, #x);                         \
                printk(fmt,##arg);\
                BUG();                                                  \
        }                                                               \
} while (0)
#else
#define ASSERT(x) do { } while (0)
#define ASSERT_PRINT(x,fmt,...) do { } while (0)

#endif

#endif/*DEBUG*/

#endif
