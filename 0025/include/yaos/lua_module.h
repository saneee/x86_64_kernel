#ifndef _YAOS_LUA_MODULE_H
#define _YAOS_LUA_MODULE_H
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#define LUA_MOD_ERR_OK 0
typedef enum lua_modeventtype {
  LUA_MOD_INSTALL=1,
} lua_modeventtype_t;

struct lua_module;


typedef struct lua_module *lua_module_t;
typedef int (*lua_modeventhand_t) (lua_State * L, unsigned long );
struct lua_moduledata {
    const char *name;           /* module name */
    lua_modeventhand_t evhand;      /* event handler */
} __packed;
typedef struct lua_moduledata lua_moduledata_t;

#define DECLARE_LUA_MODULE(name,func) \
       static int func(lua_State *L,ulong)__attribute__((__used__));\
       static struct lua_moduledata _lua_module_data_##name   \
        __attribute__((__used__))\
        __attribute__((section(".lua_module_data")))={\
      #name,func}

#define LUA_FILE_START(name) _binary_lua_##name##_lua_start
#define LUA_FILE_END(name) _binary_lua_##name##_lua_end
#define LUA_FILE_SIZE(name) _binary_lua_##name##_lua_size
#define DECLARE_LUA_FILE(name) \
  extern char  LUA_FILE_START(name)[];\
  extern char  LUA_FILE_SIZE(name)[]
#define LOAD_LUA_FILE(L,name) \
  DECLARE_LUA_FILE(name); \
  printk("%s,start:%lx,size:%lx\n",#name,LUA_FILE_START(name),LUA_FILE_SIZE(name));\
  luaL_loadbuffer(L,LUA_FILE_START(name),(size_t)(LUA_FILE_SIZE(name)),#name);\
  lua_pcall(L, 0, LUA_MULTRET, 0)
struct lua_poll;
typedef int (*lua_pollhandle_t) (struct lua_poll*);

struct lua_poll{
    struct lua_poll *pnext;
    lua_pollhandle_t  handle;
    struct thread_struct *thread;
    void *data;
    lua_State *L;
    int callback;
    int ret;
};
int lua_new_promise(lua_State *L);
#define LIGHTU_MASK (((u64)1 << 47) - 1)

#define __V2LIGHTU(p) ((u64)(p)&LIGHTU_MASK)
#define __LIGHTU2V(v) ((u64)(v)|~LIGHTU_MASK)
#include <yaos/assert.h>
static inline void * v2lightu(void* p)
{
    
    ASSERT((u64)p & ~LIGHTU_MASK);
    return (void *)__V2LIGHTU(p);
}
static inline void * lightu2v(void *p)
{
    ASSERT(((u64)p & ~LIGHTU_MASK)==0);
    return (void *)__LIGHTU2V(p);
}
#endif

