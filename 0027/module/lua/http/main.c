#include <types.h>
#include <yaos/module.h>
#include <yaos/sysparam.h>
#include <yaos/yaoscall.h>
#include <yaos/percpu.h>
#include <yaos/init.h>

#include <yaos/errno.h>
#include <yaos/printk.h>
#include <yaos/lua_module.h>
#include "util.h"
extern int ngx_lua_inject_content(lua_State *L);
extern void http_lua_inject_misc_api(lua_State *L);

static int ngx_lua_version(lua_State *L)
{
    lua_pushstring(L,"0.1.2");
    return 1;
}
static int ngx_lua_say(lua_State *L)
{
    ssize_t vga_write(void *p, size_t s);

    int argc = lua_gettop(L);
    if (argc != 1) return luaL_error(L, "ngx.say: expecting one argument");
    uchar *p;
    size_t len;
    p = (uchar *) luaL_checklstring(L, 1, &len);
    size_t w = vga_write(p, len);
    //printk("cpu_base:%016lx\n",this_cpu_read(this_cpu_off));
    lua_pushnumber(L, (lua_Number) w);
    return 1;

    return 0;        
}
__used static int ngx_main(lua_State * L, ulong event)
{
    if (event == LUA_MOD_INSTALL) {
        http_lua_init_vm(L);

        lua_createtable(L, 0 /* narr */, 99 /* nrec */);    /*yaos.* */
        lua_pushcfunction(L, ngx_lua_version);
        lua_setfield(L, -2, "version");
        lua_pushcfunction(L, ngx_lua_say);
        lua_setfield(L, -2, "say");
        ngx_lua_inject_content(L);
        http_lua_inject_misc_api(L);

        lua_setglobal(L, "ngx");

    }
    return LUA_MOD_ERR_OK;

}
DECLARE_LUA_MODULE(ngx, ngx_main);

