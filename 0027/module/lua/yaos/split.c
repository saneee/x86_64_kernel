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
static inline bool __isblank(char c)
{
    return c==' '|| c=='\t';
}
//yaos.split(str,search)
//search max length 8
//escape char % only suport at first char 
static int yaos_split(lua_State *L)
{
    int n = 1;
    int pos = 0;
    size_t len,searchlen;
    const char * src = lua_tolstring(L,1,&len);
    const char * search = lua_tolstring(L, 2, &searchlen);
    if (searchlen>8) return luaL_error(L, "yaos.split #2 max length is 8");
    bool isescape = false;
    if (search[0]=='%') isescape = true;
    if (isescape && searchlen!=2) return luaL_error(L, "yaos.split #2 escape search only support %s"); 
    if (isescape) {
        if(search[1]=='s') {
            for(int i=0; i<len; ++i){
                if(__isblank(src[i])) { 
                    lua_pushlstring(L, src+pos, i-pos);
                    pos = i+1;
                    ++n;
                }
            }
        } else {
            return luaL_error(L, "yaos.split #2 escape search only support %s");

        }
    } else {
        switch(searchlen) {
            case 0:
                for(int i=0; i<len; ++i) {
                   lua_pushlstring(L, src+pos, 1);
                   pos++;
                }
                n+=len;
                break;
            case 1:
            do {
                 char c = search[0];
                 for(int i=0; i<len; ++i){
                     if(src[i]==c) {
                         lua_pushlstring(L, src+pos, i-pos);
                         pos = i+1;
                         ++n;
                     }
                 }
            } while (0);
                 break;
            case 2:
            do {
                 u16 w = *((u16*)search);
                 for(int i=0; i<len-1; ++i){
                     if(*((u16 *)&src[i])==w) {
                         lua_pushlstring(L, src+pos, i-pos);
                         i++;
                         pos = i+1;
                         ++n;
                     }
                 }
            }while(0);
                 break;
            case 4:
            do{
                 u32 w = *((u32*)search);
                 for(int i=0; i<len-3; ++i){
                     if(*((u32 *)&src[i])==w) {
                         lua_pushlstring(L, src+pos, i-pos);
                         i+=3;
                         pos = i+1;
                         
                         ++n;
                     }
                 }
            }while(0);  
                 break;
            case 8:
            do {
                 u64 w = *((u64*)search);
                 for(int i=0; i<len-7; ++i){
                     if(*((u64 *)&src[i])==w) {
                         lua_pushlstring(L, src+pos, i-pos);
                         i+=7;
                         pos = i+1;
                         ++n;
                     }
                 }
            }while(0);
                 break;
            default:
                 for(int i=0; i<len-searchlen-1; ++i){
                     if (strncmp(&src[i],search,searchlen)==0) {
                         lua_pushlstring(L, src+pos, i-pos);
                         i+=searchlen-1;
                         pos = i+1;
                         ++n;
                     }
                 }

            }
            

        
    } 
    lua_pushlstring(L, src+pos, len-pos);
    return n;
    
}
int yaos_lua_inject_split(lua_State *L)
{
    lua_pushcfunction(L, yaos_split);
    lua_setfield(L, -2, "split");

    return LUA_MOD_ERR_OK;
}

