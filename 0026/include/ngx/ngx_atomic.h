#ifndef _NGX_ATOMIC_H
#define _NGX_ATOMIC_H
#include <ngx_config.h>
#include <ngx_core.h>
#if (NGX_PTR_SIZE == 8)
#define NGX_ATOMIC_T_LEN            (sizeof("-9223372036854775808") - 1)
#else
#define NGX_ATOMIC_T_LEN            (sizeof("-2147483648") - 1)
#endif
typedef int64_t                     ngx_atomic_int_t;
typedef uint64_t                    ngx_atomic_uint_t;
typedef volatile ngx_atomic_uint_t  ngx_atomic_t;
#define NGX_ATOMIC_T_LEN            (sizeof("-9223372036854775808") - 1)


#endif
