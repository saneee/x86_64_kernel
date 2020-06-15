#include <types.h>
#include <yaos/module.h>
#include <yaos/sysparam.h>
#include <yaos/yaoscall.h>
#include <yaos/percpu.h>
#include <yaos/init.h>

#include <yaos/errno.h>
#include <yaos/printk.h>
#include <yaos/lua_module.h>

extern int yaos_lua_inject_time(lua_State *L);
static int yaos_lua_version(lua_State *L)
{
    lua_pushstring(L,"0.1.2");
    return 1;
}
static int yaos_lua_say(lua_State *L)
{
    ssize_t vga_write(void *p, size_t s);

    int argc = lua_gettop(L);
    if (argc != 1) return luaL_error(L, "expecting one argument");
    uchar *p;
    size_t len;
    p = (uchar *) luaL_checklstring(L, 1, &len);
    size_t w = vga_write(p, len);
    lua_pushnumber(L, (lua_Number) w);
    return 1;
}
__used static int yaos_main(lua_State * L, ulong event)
{
    if (event == LUA_MOD_INSTALL) {
        lua_createtable(L, 0 /* narr */, 99 /* nrec */);    /* ngx.* */
        lua_pushcfunction(L, yaos_lua_version);
        lua_setfield(L, -2, "version");
        lua_pushcfunction(L, yaos_lua_say);
        lua_setfield(L, -2, "say");

        yaos_lua_inject_time(L);
        lua_setglobal(L, "yaos");


    }
    return LUA_MOD_ERR_OK;

}
DECLARE_LUA_MODULE(yaos, yaos_main);
