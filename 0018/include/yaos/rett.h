#ifndef _YAOS_RETT_H
#define _YAOS_RETT_H
struct ret_val_errno {
    unsigned long v;
    long e;
};
typedef struct ret_val_errno ret_t;
struct double_ret_struct {
    unsigned long v1;
    unsigned long v2;
};
typedef struct double_ret_struct double_ret;

#define RETVAL(ret,type) ((type)ret.v)
#define RETERRNO(ret) (ret.e)
#endif

