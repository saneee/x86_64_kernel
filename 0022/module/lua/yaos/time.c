#include <types.h>
#include <yaos/module.h>
#include <yaos/sysparam.h>
#include <yaos/yaoscall.h>
#include <yaos/percpu.h>
#include <yaos/init.h>

#include <yaos/errno.h>
#include <yaos/printk.h>
#include <yaos/lua_module.h>
#include <yaos/time.h>
static int yaos_lua_uptime(lua_State *L)
{
    ulong uptime = hrt_uptime();
    /*nsec to msec */
    lua_pushnumber(L, (lua_Number) (uptime/1000000UL));
    return 1;

}
int yaos_lua_inject_time(lua_State *L)
{
    lua_pushcfunction(L, yaos_lua_uptime);
    lua_setfield(L, -2, "uptime");
    return LUA_MOD_ERR_OK;
}
