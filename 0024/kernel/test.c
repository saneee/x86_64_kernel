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
void set_data_break(ulong addr)
{
    ulong dr7 = native_get_debugreg(7);
    set_debugreg(dr7|0x90001,7);
    set_debugreg(addr,0);
    printk("set data break:%lx,rsp:%lx\n",addr,read_rsp()); 
}
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
static void test_nsec (u64 nownsec,void *p)
{
    printk("test_nsec:%ld\n",nownsec/1000000000UL);
    //lua_getglobal(L,"cb1");
//    lua_pushstring(L,"1111");
  //  lua_call(L, 1, 1);
 
}
static void test_nsec2 (u64 nownsec,void *p)
{
    printk("test_nsec2:%ld\n",nownsec/1000000000UL);

//    lua_getglobal(L,"cb2");
 //   lua_pushstring(L,"2222");
   // lua_call(L, 1, 1);
}
static int test_init(bool isbp)
{
    char buf[4096];
    if(!isbp)return 0;
    set_data_break( (ulong)this_cpu_ptr(&this_cpu_off));
    printk("current:%s, stack:%lx,size:%lx,buf:%lx\n",current,current->stack_addr,current->stack_size,buf);
    set_timeout_nsec(1000000000,test_nsec,(char *)10);
    set_timeout_nsec(1500000000,test_nsec2,(char *)10);
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
     LOAD_LUA_FILE(L,dump);

     LOAD_LUA_FILE(L,test2);

//    lua_close(L);
//    set_timeout_nsec(1000000000,test_nsec,(char *)10);
 //   set_timeout_nsec(1500000000,test_nsec2,(char *)10);
    return 0;
}


late_initcall(test_init);

