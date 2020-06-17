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
static int lua_callback = LUA_REFNIL;
static void timeout_cb (u64 nownsec,void *p)
{
    lua_State *L = (lua_State *)p;

    printk("timeout_cb:%ld,p:%lx,%d\n",nownsec/1000000000UL,p,lua_callback);
    lua_rawgeti(L, LUA_REGISTRYINDEX, lua_callback);
    //lua_getglobal(L, "cb2");
    lua_pushnumber(L,nownsec/1000000000UL);
    lua_call(L, 1, 0);

//    lua_getglobal(L,"cb2");
 //   lua_pushstring(L,"2222");
   // lua_call(L, 1, 1);
}

static int yaos_lua_set_timeout(lua_State *L)
{


    int argc = lua_gettop(L);
    if (argc != 2) return luaL_error(L, "expecting two argument");
    lua_callback = luaL_ref(L, LUA_REGISTRYINDEX);//LUA_REGISTRYINDEX);
    ulong timeout = (ulong)lua_tonumber(L,2);
    //lua_gettable(L, LUA_REGISTRYINDEX);
    lua_getglobal(L,"_G");
    const char *str = lua_tostring(L,-1);
    if (str)printf("%s\n",str);
    printk("timeout:%lx,cpu_base:%lx\n",timeout,this_cpu_read(this_cpu_off));
    set_timeout_nsec(timeout*1000000UL,timeout_cb,L);
    return  0;
}
int yaos_lua_inject_time(lua_State *L)
{
    lua_pushcfunction(L, yaos_lua_uptime);
    lua_setfield(L, -2, "uptime");
    lua_pushcfunction(L, yaos_lua_set_timeout);
    lua_setfield(L, -2, "set_timeout");

    return LUA_MOD_ERR_OK;
}
