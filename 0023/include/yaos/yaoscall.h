#ifndef _YAOS_YAOSCALL_H
#define _YAOS_YAOSCALL_H
#include <yaos/rett.h>
#include <yaos/types.h>
#include <yaos/compiler.h>
#include <asm/pgtable.h>
#define YAOSCALL_TABLE_ADDR    (KERNEL_BASE+0x9000)

enum {
    YAOS_yaoscall,
    YAOS_page_alloc,
    YAOS_page_free,
    YAOS_heap_alloc,
    YAOS_heap_free,
    YAOS_malloc,
    YAOS_mfree,
    YAOS_max_physical_mem,
    YAOS_alloc_at,
    YAOS_free_at,
    YAOS_map_p2v,
    YAOS_map_flags_at,
    YAOS_map_copy,
    YAOS_alloc_linear,
    YAOS_free_linear,
    YAOS_readv,
    YAOS_writev, 
    YAOS_lseek,
    YAOS_lua_modules_init,
    MAX_YAOSCALL
};

#define ERR_YAOSCALL_NONE_EXIST -1
#define ERR_YAOSCALL_NO_MEM	-2
#define YAOSCALL_OK		0
typedef ret_t(*yaoscall_t) (u64 arg1, ...);

//static sysfunc_t *psyscall_table=(sysfunc_t *)YAOSCALL_TABLE_ADDR;
#define pyaoscall_table ((void* *)YAOSCALL_TABLE_ADDR)
#define yaoscall_p(arg1) (pyaoscall_table[arg1])
static inline void regist_yaoscall(int id, void *pfunc)
{
    pyaoscall_table[id] = pfunc;
}

#ifdef __KERNEL__
struct yaoscall_st {
    long id;
    yaoscall_t func;
} __packed;

#define DECLARE_YAOSCALL(id,func) \
       static struct yaoscall_st _yaoscall_data_##func   \
        __attribute__((__used__))\
        __attribute__((section(".yaoscall_data")))={\
      id,(yaoscall_t)func}
#define GET_YAOSCALL(id,func)\
         (&_yaoscall_data_##func)
#endif //__KERNEL__
#endif
