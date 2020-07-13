#include <types.h>
#include <yaos/module.h>
#include <yaos/sysparam.h>
#include <yaos/yaoscall.h>
#include <yaos/percpu.h>
#include <yaos/init.h>

#include <yaos/errno.h>
#include <yaos/printk.h>
#include <yaos/lua_module.h>
#include <yaoscall/lua.h>
#include <asm/cpu.h>

#if 0
#define DEBUG_PRINT printk
#else
#define DEBUG_PRINT inline_printk
#endif
#define IS_LUA_JIT_ON	1
DECLARE_MODULE(lua, 0, main);
DEFINE_PER_CPU(lua_State *,vmL);
const char *getenv(const char *name)
{
    return NULL;
}
void setpath(void)
{
}
void *fopen64()
{
    return NULL;
}
int fclose(FILE *name)
{
    return 0;
}
int luaopen_cjson(lua_State *l);
static const luaL_Reg lua_module[] = {
  { "cjson",                 luaopen_cjson },
  { NULL,               NULL }
};

static int lua_package_loader_yaos(lua_State *L)
{
  const char *name = luaL_checkstring(L, 1);
  printk("package_loader_yaos:%s\n",name);
  const luaL_Reg *lib;
  for (lib = lua_module; lib->func; lib++) {
      if (strcmp(lib->name, name)==0) {
          lua_pushcfunction(L, lib->func);
	  return 1;
      }
  }
  if( 0==load_lua_file(L, (char *)name)) return 1;
  lua_pushnil(L);
  return 1;
}

static void add_lua_loader(lua_State *L, lua_CFunction func)
{
    lua_getglobal(L, "package");                 /* L: package */
    lua_getfield(L, -1, "loaders");                /* L: package, loaders */
    // insert loader into index 2
    lua_pushcfunction(L, func);                  /* L: package, loaders, func */
    for (int i = lua_objlen(L, -2) + 1; i > 2; --i)
    {
      lua_rawgeti(L, -2, i - 1);                /* L: package, loaders, func, function */
      // we call lua_rawgeti, so the loader table now is at -3
      lua_rawseti(L, -3, i);                  /* L: package, loaders, func */
    }
    lua_rawseti(L, -2, 2);                    /* L: package, loaders */
    // set loaders into package
    lua_setfield(L, -2, "loaders");                /* L: package */
    lua_pop(L, 1);
}

__used __init static int main(module_t m, ulong t, void *arg)
{
    modeventtype_t env = (modeventtype_t) t;
    if (env == MOD_BPLOAD) {
        init_fpu();
        lua_State *L = lua_open();
        if (L) {
            yaos_lua_modules_init(L);
            DO_LUA_FILE(L,dump);
            //DO_LUA_FILE(L,split);
            add_lua_loader(L, lua_package_loader_yaos);
            //luaopen_cjson(L);
            this_cpu_write(vmL, L);
            printk("vmL:%016lx\n",L);
        }

    } else if(env == MOD_APLOAD) {
       init_fpu();
       lua_State *L = lua_open();
       if (L) {
           yaos_lua_modules_init(L);
           LOAD_LUA_FILE(L,dump);
           this_cpu_write(vmL, L);
           printk("ap vmL:%016lx\n",L);
       }

    }
    return MOD_ERR_OK;          //MOD_ERR_NEXTLOOP;
}

static const luaL_Reg yaos_lj_lib_load[] = {
  { "",                 luaopen_base },
  { LUA_LOADLIBNAME,    luaopen_package },
  { LUA_TABLIBNAME,     luaopen_table },
 // { LUA_IOLIBNAME,      luaopen_io },
//  { LUA_OSLIBNAME,      luaopen_os },
  { LUA_STRLIBNAME,     luaopen_string },
  { LUA_MATHLIBNAME,    luaopen_math },
  { LUA_DBLIBNAME,      luaopen_debug },
#if IS_LUA_JIT_ON
  { LUA_JITLIBNAME,  luaopen_jit },
#endif
  { NULL,               NULL }
};
static inline ulong myabs(ulong a, ulong b)
{
    return a > b ? a - b : b - a;
}


__used static int dummy_lua(lua_State * L, ulong event)
{
    return LUA_MOD_ERR_OK;
}
DECLARE_LUA_MODULE(dummy1, dummy_lua);
DECLARE_LUA_MODULE(dummy2, dummy_lua);
static int lua_yaos_openlibs(lua_State *L)
{
 const luaL_Reg *lib;
    for (lib = yaos_lj_lib_load; lib->func; lib++) {
      lua_pushcfunction(L, lib->func);
      lua_pushstring(L, lib->name);
      lua_call(L, 1, 0);
    }
    return 0;
}
static int lua_install_modules(lua_State *L)
{
    extern lua_moduledata_t _lua_module_data_start[];   //kernel64.ld
    extern lua_moduledata_t _lua_module_data_end[];
    lua_yaos_openlibs(L);
    ulong step =
        myabs((ulong) & _lua_module_data_dummy1, (ulong) & _lua_module_data_dummy2);

    lua_moduledata_t *p = _lua_module_data_start;
    while (p < _lua_module_data_end) {
        if (!p->name || !p->evhand) {
            DEBUG_PRINT("zero module:%lx,%s,%lx\n", p, p->name, p->evhand );

            p = (lua_moduledata_t *) ((ulong) p + step);
            continue;
        }
        (*p->evhand)(L, LUA_MOD_INSTALL);
        p = (lua_moduledata_t *) ((ulong) p + step);
    }
   return OK;
}
DECLARE_YAOSCALL(YAOS_lua_modules_init, lua_install_modules);
