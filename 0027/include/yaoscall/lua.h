#ifndef _SYSCALL_API_H
#define _SYSCALL_API_H

#include <types.h>
#include <yaos/yaoscall.h>
#include <yaos/rett.h>
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#define LUA_MOD_ERR_OK 0
static inline int yaos_lua_modules_init(lua_State *L)
{
    int (*pfunc)(lua_State *L)=yaoscall_p(YAOS_lua_modules_init);
    return (*pfunc)(L);
}
#endif

