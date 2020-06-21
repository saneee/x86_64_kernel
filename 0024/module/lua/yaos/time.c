#include <types.h>
#include <yaos/sched.h>

#include <yaos/module.h>
#include <yaos/sysparam.h>
#include <yaos/yaoscall.h>
#include <yaos/percpu.h>
#include <yaos/init.h>

#include <yaos/errno.h>
#include <yaos/printk.h>
#include <yaos/lua_module.h>
#include <yaos/time.h>
#include <yaoscall/malloc.h>
int lua_poll_new_promise(lua_State *L, struct lua_poll *p, void *cb);

static int yaos_lua_uptime(lua_State *L)
{
    ulong uptime = hrt_uptime();
    /*nsec to msec */
    lua_pushnumber(L, (lua_Number) (uptime/1000000UL));
    return 1;

}
static void timeout_cb (u64 nownsec,void *p)
{
    struct lua_poll *p_poll = (struct lua_poll *)p;
    p_poll->data = (void *)(nownsec / 1000000000UL);
    p_poll->ret = OK;
    thread_add_poll(p_poll,POLL_LVL_LUA);

    //printk("timeout_cb:%ld,p:%lx,id:%d\n",nownsec/1000000000UL,p,p_poll->callback);
    
}
static int poll_callback(struct lua_poll *p)
{
    lua_State *L = p->L;
    printk("poll_callback:%lx,callbackid:%d\n",p, p->callback);
    if (p->callback!=-1) {

        lua_rawgeti(L, LUA_REGISTRYINDEX, p->callback);
        if (lua_isfunction(L, -1)) {
            lua_pushnumber(L,(ulong)p->data);
            lua_call(L,1,0);
        } else {
            //lua_pushnumber(L,(ulong)p->data);
            //yaos_lua_promise_fire(L);
            ASSERT(0);//now use callback all
        }
        luaL_unref(L, LUA_REGISTRYINDEX, p->callback);

    }
    yaos_mfree(p);
    return 0;
}
static void yaos_lua_create_timer(struct lua_poll *p_poll)
{
    ulong timeout = (ulong)p_poll->data;
    set_timeout_nsec(timeout*1000000UL,timeout_cb,p_poll);
     
}
static int yaos_lua_set_timeout(lua_State *L)
{

    int argc = lua_gettop(L);
    if (argc <1) return luaL_error(L, "expecting one or two argument");
    ulong timeout =  (ulong)lua_tonumber(L,argc>1?2:1);
    struct lua_poll *p_poll = yaos_malloc(sizeof(struct lua_poll));
    if (!p_poll) return ENOMEM;
    p_poll->L = L;
    p_poll->handle = poll_callback;
    p_poll->thread = current;

    if (argc >1) {
        lua_pop(L, 1);//pop timeout
        p_poll->callback = luaL_ref(L, LUA_REGISTRYINDEX);
        set_timeout_nsec(timeout*1000000UL,timeout_cb,p_poll);
        return 0;

    } else {
        p_poll->data = (void *)timeout;
        return lua_poll_new_promise(L, p_poll, yaos_lua_create_timer);

    }
}
int yaos_lua_inject_time(lua_State *L)
{
    lua_pushcfunction(L, yaos_lua_uptime);
    lua_setfield(L, -2, "uptime");
    lua_pushcfunction(L, yaos_lua_set_timeout);
    lua_setfield(L, -2, "set_timeout");

    return LUA_MOD_ERR_OK;
}
