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
#if 1
#define DEBUG_PRINT printk
#define DEBUG_DUMP(a,b) dump(L,a,b)
#else
#define DEBUG_PRINT inline_printk
#define DEBUG_DUMP(a,b)
#endif
int yaos_lua_ret_promise(lua_State *L);

static void reject(lua_State *L);
static void resolve(lua_State *L);
static int thennext(lua_State *L);
static void doResolve(lua_State *L);
static int Handler(lua_State *L);
static void handle(lua_State *L);
enum {
    PENDING = 0,
    RESOLVED,
    REJECTED,
    RESOLVING,
};
static inline bool has_function(lua_State *L,const char *funcname,int pos)
{
    bool hasfunc = false;
    lua_getfield(L, pos, funcname);
    if (lua_type(L,-1) == LUA_TFUNCTION) {
          hasfunc = true;
    }
    lua_pop(L,1);
    return hasfunc;
}
static inline bool is_promise(lua_State *L,int pos)
{
    return (lua_type(L,pos)==LUA_TTABLE) && has_function(L,"next",pos);
}
__used static void dump(lua_State *L,const char *str, int pos)
{
    lua_getglobal(L,"show_dump");
    DEBUG_PRINT(str,lua_type(L,-1),lua_type(L,-2));
    lua_pushvalue(L, pos);
    lua_call(L,1,0);

}
/*
Promise(fn)
*/
static int _idx = 0;
int lua_new_promise(lua_State *L){
    int argc = lua_gettop(L);
    printk("lua_new_promise:%d,%d,%d\n",argc,lua_type(L,1),lua_type(L,-1));
    lua_createtable(L, 0 /* narr */, 16 /* nrec */);
    lua_pushinteger(L, PENDING);
    lua_setfield(L, -2, "_state");
    lua_pushinteger(L, 0);
    lua_setfield(L, -2, "_deferredState");
    
    lua_pushcfunction(L, thennext);
    lua_setfield(L, -2, "next");
    lua_pushinteger(L, _idx++);
    lua_setfield(L, -2, "_id");
    if (argc > 0) {
       lua_pushvalue(L, -1);
       lua_pushvalue(L, 1);
       if (lua_type(L, 1) ==  LUA_TFUNCTION)
         doResolve(L);
       lua_pop(L, 2);
    }
    return  1;
    
}
/*
thennext(self, onFulfilled, onRejected)
*/
static int thennext(lua_State *L)
{
    int argc = lua_gettop(L);
    if (argc <3) {
        lua_pushnil(L);
    }
    DEBUG_PRINT("thennext:%d\n", argc);
    lua_pushcfunction(L, lua_new_promise);
    lua_call(L, 0, 1);
    Handler(L);
    //-1 new handler -2 new promise 
    lua_pushvalue(L, 1); //push self
    lua_pushvalue(L, -2);
    handle(L);
    lua_pop(L, 3); //return new promise
    return 1;
}
/*
   Handler(onFulfilled, onRejected, promise) 
-3:onFullField
-2:onRejected
-1:promise
*/
static int Handler(lua_State *L)
{
    lua_createtable(L,0,3);
    lua_pushvalue(L, -2);
    lua_setfield(L, -2, "promise");
    lua_pushvalue(L, -3);
    lua_setfield(L, -2, "onRejected");
    lua_pushvalue(L, -4);
    lua_setfield(L, -2, "onFulfilled");
    return 1;    
}
/*
finale(self)
*/
static void finale(lua_State *L)
{
    DEBUG_DUMP("finale self:%d,%d\n",-2);
    int top = lua_gettop(L);
    lua_getfield(L, -1, "_deferredState");
    int deferredState = lua_tointeger(L, -1);
    lua_pop(L, 1);
    DEBUG_PRINT("finale deferredState:%d,top:%d,%d\n",deferredState, top, lua_gettop(L));
    if (deferredState == 1) {
       lua_pushvalue(L, top); 
       lua_getfield(L, -1, "_deferreds");
       handle(L);
       lua_pushnil(L);
       lua_setfield(L, top, "_deferreds"); 
       lua_settop(L, top);
    }
    if (deferredState == 2) {
       lua_getfield(L, -1, "_deferreds");
       int index = lua_gettop(L);
       lua_pushnil(L);
       while (lua_next(L, index)) {
           lua_pushvalue(L, index-1);//self
           lua_pushvalue(L, -2);
           handle(L);
           lua_pop(L, 3); //pop value and 2 params 
       }
       lua_settop(L, top);

    }
   
}
static int my_resolve(lua_State *L)
{
    int index = lua_tointeger(L,lua_upvalueindex(1));
    DEBUG_PRINT("my_resolve:%d\n",index);
    lua_rawgeti(L, LUA_REGISTRYINDEX, index);
    int done = lua_tointeger(L, -1);
    int promise = lua_tointeger(L,lua_upvalueindex(2));

    DEBUG_PRINT("done:%d,promise:%d\n",done,promise);
    lua_pop(L, 1);
    if (done) goto clean;
    
    lua_pushinteger(L,1);
    lua_rawseti(L, LUA_REGISTRYINDEX, index); //done = true
    lua_rawgeti(L, LUA_REGISTRYINDEX, promise);
    lua_pushvalue(L,1);
    resolve(L);
clean:
    luaL_unref(L, LUA_REGISTRYINDEX, index);
    luaL_unref(L, LUA_REGISTRYINDEX, promise);

    return 0;
}
static int my_reject(lua_State *L)
{
    int index = lua_tointeger(L,lua_upvalueindex(1));

    lua_rawgeti(L, LUA_REGISTRYINDEX, index);
    int done = lua_tointeger(L, -1);
    lua_pop(L, 1);
    if (done) return 0;

    lua_pushinteger(L,1);
    lua_rawseti(L, LUA_REGISTRYINDEX, index);
    int promise = lua_tointeger(L,lua_upvalueindex(2));
    lua_rawgeti(L, LUA_REGISTRYINDEX, promise);
    lua_pushvalue(L, 1);
    reject(L);
    return 0;
}
/*
doResolve(promise, fn);
-2: promise
-1: fn
*/
static void doResolve(lua_State *L){
    lua_pushinteger(L, 0);
    int index = luaL_ref(L, LUA_REGISTRYINDEX);
    lua_pushvalue(L, -2);
    int promise = luaL_ref(L, LUA_REGISTRYINDEX);
    DEBUG_PRINT("doResolve:%d,%d\n",index, promise);
    lua_pushvalue(L, -1);
    lua_pushvalue(L, -3);
    lua_pushinteger(L, index);
    lua_pushinteger(L, promise);
    lua_pushcclosure(L, my_resolve, 2);
    lua_pushinteger(L, index);
    lua_pushinteger(L, promise);
    lua_pushcclosure(L, my_reject, 2);
    DEBUG_PRINT("top:%d,%d,%d,%d\n",lua_type(L,-1),lua_type(L,-2),lua_type(L,-3),lua_type(L,-4)); 
    if (0==lua_pcall(L,3,0,0)) {
        
    } else {
        lua_rawgeti(L, LUA_REGISTRYINDEX, index);
        int done = lua_tointeger(L, -1);
        lua_pop(L, 1);
        if (!done) {
           lua_pushinteger(L, 1);
           lua_rawseti(L, LUA_REGISTRYINDEX, index);
                
        }     
        lua_pushvalue(L, -2);
        lua_newtable(L);
        reject(L);
        lua_pop(L, 2);

    }    
}
/*
resolved(self, newvalue)
-2:self
-1:value

*/

static void resolve(lua_State *L)
{
   int state = 1;
   int top = lua_gettop(L);
   DEBUG_PRINT("resolve is_promise:%d\n",is_promise(L, -1));
   if (is_promise(L, -1)) {

       lua_getfield(L, -1, "next");
       lua_getfield(L, -3, "next");
       if (false && !lua_rawequal(L, -1, -2)) {
           //-1 value.next, -2 value
           DEBUG_PRINT("!lua_rawequal(L, -1, -2),%d,%d",lua_type(L,-1),lua_type(L,-2));
           lua_pop(L, 1);
           doResolve(L);
           lua_pop(L, 1);
           return;
       } else {
           lua_pop(L, 2);
           state = RESOLVING;
       }
   }
   lua_pushstring(L, "_state");
   lua_pushinteger(L, state);
   lua_rawset(L, top - 1);
   lua_pushstring(L, "_value");
   lua_pushvalue(L, top);
   lua_rawset(L, top -1);
   lua_pushvalue(L, top -1);
   finale(L);
   lua_settop(L, top); 
}
/*
reject(self, newvalue)
-2:self
-1:value

*/

static void reject(lua_State *L)
{
    DEBUG_PRINT("reject\n");
    lua_pushinteger(L, REJECTED);
    lua_setfield(L, -3, "_state");
    lua_pushvalue(L, -1);
    lua_setfield(L, -3, "_value");
    lua_pushvalue(L, -2);
    finale(L);
    lua_pop(L, 1);
}

static int error_handle(lua_State *L)
{
    DEBUG_DUMP("error_handle:%d,%d\n",1);

    int index = lua_tointeger(L,lua_upvalueindex(1));

    lua_rawgeti(L, LUA_REGISTRYINDEX, index);
    lua_pushvalue(L, 1);
    reject(L);
    lua_settop(L,1);
    luaL_unref(L, LUA_REGISTRYINDEX, index);
    return 1;
 
}
/*
handleResolved(self,deferred)
-2:self
-1:deferred

*/
static void handleResolved(lua_State *L)
{
    DEBUG_PRINT("handleResolved\n");
    lua_getfield(L, -2, "_state");
    int state = lua_tointeger(L,-1);
    lua_pop(L, 1);
    int old = lua_gettop(L);
    if (state == RESOLVED) {
        lua_getfield(L, -1, "onFulfilled");
    } else {
        lua_getfield(L, -1, "onRejected");
    }
    if (lua_type(L, -1) == LUA_TNIL) {
        lua_pop(L, 1);
        lua_getfield(L, -1, "promise");
        lua_getfield(L, -3, "_value");
DEBUG_PRINT("state:%d\n",state);
        if (state == RESOLVED)
            resolve(L); //resolve(deferred.promise, self._value);
        else
            reject(L);
        lua_pop(L, 2);
        return;
    }
    lua_getfield(L, old-1, "_value");
    lua_pushvalue(L, old-1);//self
    int promise = luaL_ref(L, LUA_REGISTRYINDEX);
    lua_pushinteger(L, promise);
    lua_pushcclosure(L, error_handle, 1);
    //-1 closure -2 value -3 resolve
    lua_insert(L, old);
    if (0 == lua_pcall(L,1,1,old)) {
        lua_remove(L, old);
        lua_getfield(L, -2, "promise"); // -1 retval ,-2 deferred
        lua_pushvalue(L, -2); //-1 retv -2 promise -3 retv -4 deferred -5 self
DEBUG_PRINT("0==lua_pcall\n");
        resolve(L);
        lua_pop(L, 3);
    } else {
DEBUG_PRINT("0!=lua_pcall\n");
        lua_remove(L, old);
        lua_getfield(L, -2, "promise");
        lua_newtable(L);
        //reject(L);
        lua_pop(L, 3);
    }
}
/*
handle(self,deferred)
-2:self
-1:deferred
*/
static void handle(lua_State *L)
{
    int top = lua_gettop(L);
    DEBUG_PRINT("handle:%d\n",top);
    DEBUG_DUMP("showdump self:%d,%d\n",-3);


    lua_pushvalue(L, -2);
    lua_getfield(L, -1, "_state");
    int state = lua_tointeger(L,-1);
    while (state ==  RESOLVING) {
        DEBUG_PRINT("state ==  RESOLVING");
        lua_pop(L, 1);
        //self = self._value
        lua_getfield(L, -1, "_value");//-1 _value -2:self
        lua_replace(L, -2);
        lua_getfield(L, -1, "_state");
        state = lua_tointeger(L,-1);
        DEBUG_PRINT(", state:%d,%d,%d\n",state,lua_type(L,-1),lua_type(L,-2)); 
    }
    lua_pop(L, 1);
    if (state == PENDING) {
       lua_getfield(L, -1, "_deferredState");
       int deferredState = lua_tointeger(L, -1);
       DEBUG_PRINT("state:%d ,deferredState:%d\n", state,deferredState);
       lua_pop(L, 1);
       if (deferredState == 0) {
           lua_pushnumber(L, 1);
           lua_setfield(L, -2, "_deferredState");
           lua_pushvalue(L, top);
           lua_setfield(L, -2, "_deferreds");//self._deferreds = deferred;

           lua_settop(L, top);
           return;

       }
       if (deferredState == 1) {
           lua_pushnumber(L, 2);
           lua_setfield(L, -2, "_deferredState");
           lua_createtable(L, 2 /* narr */, 0 /* nrec */);
           lua_getfield(L, -2, "_deferreds");
           lua_rawseti(L, -2, 1);
           // -1 table -2self, -3 deferer
           lua_pushvalue(L, -3);
           lua_rawseti(L, -2, 2);
           //self._deferreds = [self._deferreds, deferred];
           lua_setfield(L, -2, "_deferreds");
           lua_pop(L, 1);
           return;
       
       }
       lua_getfield(L, -1, "_deferreds");
       int n = lua_objlen(L, -1);
       lua_pushvalue(L, -3);
       lua_rawseti(L, -2, n+1);
       lua_pop(L, 1);
       return;   

 
    }
    lua_pushvalue(L, -2);
    // -1 deferred, -2 new self -3 deferred -4 old self
    handleResolved(L);
    lua_pop(L, 2);
}


int yaos_lua_inject_promise(lua_State *L)
{
    lua_pushcfunction(L, lua_new_promise);
    lua_setfield(L, -2, "promise");
    return 0;    
}
