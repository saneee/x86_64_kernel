#include <types.h>
#include <yaos/module.h>
#include <yaos/sysparam.h>
#include <yaos/yaoscall.h>
#include <yaos/percpu.h>
#include <yaos/init.h>

#include <yaos/errno.h>
#include <yaos/printk.h>
#include <yaos/lua_module.h>
static int lua_callback = LUA_REFNIL;

//lua_poll_fire(data)
int  lua_poll_fire(lua_State *L)
{
    printk("lua_poll_fire\n");
    lua_pushvalue(L, lua_upvalueindex(2));
    lua_pushvalue(L, -2);
    lua_call(L, 1, 0);
    return 0;
}

//new Promise(lua_poll_fn)
//lua_poll_fn(resolve,reject)
int lua_poll_fn(lua_State *L)
{
    
    struct lua_poll *p  = lightu2v(lua_touserdata(L,lua_upvalueindex(1)));
    lua_pushlightuserdata(L, v2lightu(p));
    void (*create)(struct lua_poll *p) = lightu2v(lua_touserdata(L,lua_upvalueindex(2)));
    printk("lua_poll_fn: lua_poll:%lx, create_fn:%lx\n",p, create);
    lua_pushvalue(L, -3);//resolve
    lua_pushvalue(L, -3);//reject
    lua_pushcclosure(L, lua_poll_fire, 3);
    p->callback = luaL_ref(L, LUA_REGISTRYINDEX);
    if (create) (*create)(p); 
    return 0;
}
int lua_poll_new_promise(lua_State *L, struct lua_poll *p, void *cb)
{
    printk("lua_poll_new_promise:%p,%p,%p\n",L,p,cb);
    lua_pushcfunction(L, lua_new_promise);
    lua_pushlightuserdata(L, v2lightu(p));
    lua_pushlightuserdata(L, v2lightu(cb));
    lua_pushcclosure(L, lua_poll_fn, 2);
    lua_call(L, 1, 1);
    //lua_new_promise(L);
    return 1;
}
static int setnotify(lua_State *L)
{
  lua_callback = luaL_ref(L, LUA_REGISTRYINDEX);
  return 0;
}
 
static int testnotify(lua_State *L)
{
  lua_rawgeti(L, LUA_REGISTRYINDEX, lua_callback);
  lua_call(L, 0, 0);
  return 0;
}
 
static int testenv(lua_State *L)
{
  lua_getglobal(L, "defcallback");
  lua_call(L, 0, 0);
  return 0;
}
static int test_closure(lua_State *L)
{
    int first = lua_tointeger(L,lua_upvalueindex(1));
    printk("first:%d\n",first);
    lua_rawgeti(L, LUA_REGISTRYINDEX, first);
    int now = lua_tointeger(L, -1);
    lua_pop(L,1);
    printk("now:%d\n",now);
    lua_pushinteger(L, now+1);
    lua_rawseti(L, LUA_REGISTRYINDEX, first);
    lua_rawgeti(L, LUA_REGISTRYINDEX, first);

    //lua_rawget(L, lua_upvalueindex(1));
    //first = lua_tointeger(L,lua_upvalueindex(1));
    //lua_pushinteger(L, first);
    return 1;
}
__used static int test_close2(lua_State *L)
{
    lua_call(L, 0, 1);
    printk("test_close2\n");
    return 1;
}
static int test(lua_State *L)
{
    //lua_pushcfunction(L, test_close2);
    lua_pushinteger(L, 100);
    int lua_callback = luaL_ref(L, LUA_REGISTRYINDEX);
    lua_pushinteger(L, lua_callback);
    lua_pushcclosure(L, test_closure, 1);
    lua_pushinteger(L, lua_callback);
    lua_pushcclosure(L, test_closure, 1);

    lua_call(L, 2, 1);
    printk("hi");
    return 1;
    
}
extern int yaos_lua_inject_time(lua_State *L);
extern int yaos_lua_inject_promise(lua_State *L);
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
    //printk("cpu_base:%016lx\n",this_cpu_read(this_cpu_off));
    lua_pushnumber(L, (lua_Number) w);
    return 1;
}
__used static int yaos_main(lua_State * L, ulong event)
{
    if (event == LUA_MOD_INSTALL) {
        lua_createtable(L, 0 /* narr */, 99 /* nrec */);    /*yaos.* */
        lua_pushcfunction(L, yaos_lua_version);
        lua_setfield(L, -2, "version");
        lua_pushcfunction(L, yaos_lua_say);
        lua_setfield(L, -2, "say");
        lua_pushcfunction(L, setnotify);
        lua_setfield(L, -2, "setnotify");
        lua_pushcfunction(L, testnotify);
        lua_setfield(L, -2, "testnotify");
        lua_pushcfunction(L, testenv);
        lua_setfield(L, -2, "testenv");
        lua_pushcfunction(L, test);
        lua_setfield(L, -2, "test");
        yaos_lua_inject_time(L);
        yaos_lua_inject_promise(L);
        lua_setglobal(L, "yaos");


    }
    return LUA_MOD_ERR_OK;

}
DECLARE_LUA_MODULE(yaos, yaos_main);
