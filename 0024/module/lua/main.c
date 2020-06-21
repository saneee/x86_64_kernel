#include <types.h>
#include <yaos/module.h>
#include <yaos/sysparam.h>
#include <yaos/yaoscall.h>
#include <yaos/percpu.h>
#include <yaos/init.h>

#include <yaos/errno.h>
#include <yaos/printk.h>
#include <yaos/lua_module.h>
#if 1
#define DEBUG_PRINT printk
#else
#define DEBUG_PRINT inline_printk
#endif

static const luaL_Reg yaos_lj_lib_load[] = {
  { "",                 luaopen_base },
//  { LUA_LOADLIBNAME,    luaopen_package },
  { LUA_TABLIBNAME,     luaopen_table },
 // { LUA_IOLIBNAME,      luaopen_io },
//  { LUA_OSLIBNAME,      luaopen_os },
  { LUA_STRLIBNAME,     luaopen_string },
  { LUA_MATHLIBNAME,    luaopen_math },
//  { LUA_DBLIBNAME,      luaopen_debug },
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
