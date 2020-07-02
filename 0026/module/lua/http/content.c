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
#include "common.h"
#include "lwip/init.h"
#include "lwip/pbuf.h"
extern struct pbuf * get_req_buf(struct http_state *hs);
static int ngx_lua_say(lua_State *L)
{
    http_request_t *r = http_lua_get_req(L);
    if (r == NULL) {
        return luaL_error(L, "request object not found");
    }
    int argc = lua_gettop(L);
    if (argc != 1) return luaL_error(L, "expecting one argument");
    uchar *p;
    size_t len;
    p = (uchar *) luaL_checklstring(L, 1, &len);
    err_t http_send_content(struct http_state *hs, char *p, size_t len);

    err_t err = http_send_content(r->hs, p, len);
    printk("http_send_content:%d\n",err);
    return 0;

}
static int ngx_lua_get_req(lua_State *L)
{
    http_request_t *r = http_lua_get_req(L);
    if (r == NULL) {
        return luaL_error(L, "request object not found");
    }

    struct pbuf * p = get_req_buf(r->hs);
    if (!p) {
       lua_pushnil(L);
       return 1;

    }
    printk("pbuf:%lx,total:%d,len:%d\n",p,p->tot_len, p->len);

    if (p->next == NULL) {
        printk("p->payload:%lx,len:%d\n",p->payload, p->len);
        lua_pushlstring(L, (char *) p->payload, p->len);
        return 1;

    }
    int nr = 0;
    while (p) {
        lua_pushlstring(L, (char *) p->payload, p->len);
        nr++;
        p = p->next;

    }
    return nr;
}
int ngx_lua_inject_content(lua_State *L)
{
    lua_pushcfunction(L, ngx_lua_get_req);
    lua_setfield(L, -2, "get_req");
    lua_pushcfunction(L, ngx_lua_say);
    lua_setfield(L, -2, "say");

    return LUA_MOD_ERR_OK;
}

int http_lua_content_by_file(const char *name, lua_State *L, http_request_t *r)
{
    int co_ref;
    lua_State *co;
    http_lua_ctx_t  *ctx = http_get_module_ctx(r, HTTP_LUA_MODULE); 
    if (ctx == NULL) {
       ctx = http_lua_create_ctx(r);
       if (ctx == NULL) {
           return ENOMEM;
       }
    } else {
       http_lua_reset_ctx(r, L, ctx);
    }
    if (load_lua_file(L, (char *)name) != 0) return ESRCH;
     /*  {{{ new coroutine to handle request */
    co = http_lua_new_thread(L, &co_ref);
    printk("co:%lx\n",co);
    if ( co == NULL ) {
        return ENOMEM;
    }
    lua_xmove(L, co, 1);
    http_lua_get_globals_table(co);
    lua_setfenv(co, -2);
    http_lua_set_req(co, r);
    ctx->cur_co_ctx = &ctx->entry_co_ctx;
    ctx->cur_co_ctx->co = co;
    ctx->cur_co_ctx->co_ref = co_ref;
    int rc = http_lua_run_thread(L, r, ctx, 0);

    return rc;
}
