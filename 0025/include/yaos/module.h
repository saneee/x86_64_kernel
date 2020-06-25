#ifndef _YAOS_MODULE_H
#define _YAOS_MODULE_H
#define MOD_ERR_NEXTLOOP (-1)
#define MOD_ERR_OK	0
#include <yaos/compiler.h>
typedef enum modeventtype {
    MOD_BPLOAD,
    MOD_APLOAD,
    MOD_BPSUBSYS,
    MOD_APSUBSYS,
    MOD_BPDEVICE,
    MOD_APDEVICE,
    MOD_UNLOAD,
    MOD_SHUTDOWN,
    MOD_QUIESCE
} modeventtype_t;
struct module;
typedef struct module *module_t;
typedef int (*modeventhand_t) (module_t, unsigned long, void *);
struct moduledata {
    const char *name;           /* module name */
    modeventhand_t evhand;      /* event handler */
    void *priv;                 /* extra data */
} __packed;
typedef struct moduledata moduledata_t;
typedef union modspecific {
    int intval;
    u_int uintval;
    long longval;
    u_long ulongval;
} modspecific_t;

#define DECLARE_MODULE(name,data,func) \
       static int func(module_t,ulong,void *)__attribute__((__used__));\
       static struct moduledata _module_data_##name   \
        __attribute__((__used__))\
        __attribute__((section(".module_data")))={\
      #name,func,(void*)data}

#endif
