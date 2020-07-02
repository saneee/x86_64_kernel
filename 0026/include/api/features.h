#ifndef _FEATURES_H
#define _FEATURES_H

#ifdef _ALL_SOURCE
#define _GNU_SOURCE 1
#endif

#if !defined(_POSIX_SOURCE) && !defined(_POSIX_C_SOURCE) \
 && !defined(_XOPEN_SOURCE) && !defined(_GNU_SOURCE) \
 && !defined(_BSD_SOURCE) && !defined(__STRICT_ANSI__)
#define _BSD_SOURCE 1
#define _XOPEN_SOURCE 700
#endif

#if __STDC_VERSION__ >= 199901L
#define __restrict restrict
#elif !defined(__GNUC__)
#define __restrict
#endif

#if __STDC_VERSION__ >= 199901L || defined(__cplusplus)
#ifndef __inline
#define __inline inline
#endif
#endif

#if __STDC_VERSION__ >= 201112L
#elif defined(__GNUC__)
#define _Noreturn __attribute__((__noreturn__))
#else
#define _Noreturn
#endif

#ifdef __cplusplus
#define __BEGIN_DECLS extern "C" {
#define __END_DECLS }
#define __THROW throw ()
#else
#define __BEGIN_DECLS
#define __END_DECLS
#define __THROW
#endif

#endif
