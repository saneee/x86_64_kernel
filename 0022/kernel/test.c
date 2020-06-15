#include <types.h>
#include <yaos/init.h>
#include <yaos/time.h>
#include <yaos/printk.h>
#include <yaos/smp.h>
#include <yaos/cpupm.h>
#include <yaos/tasklet.h>
#include <yaos/sched.h>
#include <asm/current.h>
#include <yaoscall/malloc.h>
#include <drivers/pci_device.h>
#include <drivers/virtio.h>
#define VIRTIO_NET_SUBID 0x1000
#include <yaoscall/lua.h>
#include <yaos/lua_module.h>
#include <stdio.h>
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
lua_State* L;

int luaadd(int x, int y)
{
    int sum;
    lua_getglobal(L,"add");
    lua_pushnumber(L, x);
 
    lua_pushnumber(L, y);
    lua_call(L, 2, 1);
    sum = (int)lua_tonumber(L, -1);
    lua_pop(L,1);
    return sum;
}
static int test_init(bool isbp)
{

    if(!isbp)return 0;
    init_fpu();
    int sum;
    L = lua_open();
    yaos_lua_modules_init(L);
    //luaL_openlibs(L);
    //luaL_dofile(L, "add.lua");
    luaL_dostring(L, "function add(x,y)\n\
        return x + y * 100\n\
    end\n\
    ");
    sum = luaadd(10, 15);
    printf("##################################LUA_RET###################\nThe sum is %d \n",sum);
    luaL_dostring(L,  "yaos.say(\"yaos_version:\"..yaos.version()..\"\\n\")\n\
    yaos.say(\"uptime_msec:\"..yaos.uptime()..\"\\n\")\n\
    ");
    LOAD_LUA_FILE(L,test);
    LOAD_LUA_FILE(L,test2);
    lua_close(L);
    return 0;
}


late_initcall(test_init);

