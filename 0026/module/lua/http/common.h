#ifndef _MOD_LUA_HTTP_COMMON_H
#define _MOD_LUA_HTTP_COMMON_H
#include "../../httpd/httpd_request.h"
typedef enum {
    HTTP_LUA_USER_CORO_NOP      = 0,
    HTTP_LUA_USER_CORO_RESUME   = 1,
    HTTP_LUA_USER_CORO_YIELD    = 2,
    HTTP_LUA_USER_THREAD_RESUME = 3
} http_lua_user_coro_op_t;


typedef enum {
    HTTP_LUA_CO_RUNNING   = 0, /* coroutine running */
    HTTP_LUA_CO_SUSPENDED = 1, /* coroutine suspended */
    HTTP_LUA_CO_NORMAL    = 2, /* coroutine normal */
    HTTP_LUA_CO_DEAD      = 3, /* coroutine dead */
    HTTP_LUA_CO_ZOMBIE    = 4, /* coroutine zombie */
} http_lua_co_status_t;


typedef struct http_lua_co_ctx_s  http_lua_co_ctx_t;

struct http_lua_co_ctx_s {
    void                    *data;      /* user state for cosockets */

    lua_State               *co;
    http_lua_co_ctx_t       *parent_co_ctx;
    int                      co_ref; /*  reference to anchor the thread
                                         coroutines (entry coroutine and user
                                         threads) in the Lua registry,
                                         preventing the thread coroutine
                                         from beging collected by the
                                         Lua GC */
    unsigned                 waited_by_parent:1;  /* whether being waited by
                                                     a parent coroutine */

    unsigned                 co_status:3;  /* the current coroutine's status */

    unsigned                 flushing:1; /* indicates whether the current
                                            coroutine is waiting for
                                            ngx.flush(true) */

    unsigned                 is_uthread:1; /* whether the current coroutine is
                                              a user thread */

    unsigned                 thread_spawn_yielded:1; /* yielded from
                                                        the ngx.thread.spawn()
                                                        call */
    unsigned                 sem_resume_status:1;
};

typedef struct http_lua_ctx_s {
    int  ctx_ref;  /*  reference to anchor
                       request ctx data in lua registry */
    http_lua_co_ctx_t    entry_co_ctx; /* coroutine context for the
                                              entry coroutine */
    http_lua_co_ctx_t   *cur_co_ctx; /* co ctx for the current coroutine */
    http_request_t      *request;
    int			exit_code;
    unsigned                 run_post_subrequest:1; /* whether it has run
                                                       post_subrequest
                                                       (for subrequests only) */

    unsigned                 waiting_more_body:1;   /* 1: waiting for more
                                                       request body data;
                                                       0: no need to wait */

    unsigned         co_op:2; /*  coroutine API operation */

    unsigned         exited:1;
    unsigned         eof:1;             /*  1: last_buf has been sent;
                                            0: last_buf not sent yet */

    unsigned         capture:1;  /*  1: response body of current request
                                        is to be captured by the lua
                                        capture filter,
                                     0: not to be captured */


    unsigned         read_body_done:1;      /* 1: request body has been all
                                               read; 0: body has not been
                                               all read */

    unsigned         headers_set:1; /* whether the user has set custom
                                       response headers */
    unsigned         mime_set:1;    /* whether the user has set Content-Type
                                       response header */

    unsigned         entered_rewrite_phase:1;
    unsigned         entered_access_phase:1;
    unsigned         entered_content_phase:1;
    unsigned         buffering:1; /* HTTP 1.0 response body buffering flag */

    unsigned         no_abort:1; /* prohibit "world abortion" via ngx.exit()
                                    and etc */

    unsigned         header_sent:1; /* r->header_sent is not sufficient for
                                     * this because special header filters
                                     * like ngx_image_filter may intercept
                                     * the header. so we should always test
                                     * both flags. see the test case in
                                     * t/020-subrequest.t */

    unsigned         seen_last_in_filter:1;  /* used by body_filter_by_lua* */
    unsigned         seen_last_for_subreq:1; /* used by body capture filter */
    unsigned         writing_raw_req_socket:1; /* used by raw downstream
                                                  socket */
    unsigned         acquired_raw_req_socket:1;  /* whether a raw req socket
                                                    is acquired */
    unsigned         seen_body_data:1;


} http_lua_ctx_t;
int http_lua_ngx_get_ctx(lua_State *L);
int http_lua_ngx_set_ctx(lua_State *L);

#endif
