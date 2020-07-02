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
#include "lwip/priv/tcp_priv.h"
struct tcp_pcb* pcb;
extern int except_count;
void set_data_break(ulong addr)
{
    ulong dr7 = native_get_debugreg(7);
    set_debugreg(dr7|0xd5190007,7);
    set_debugreg(addr,0);
    set_debugreg(addr,1);
    set_debugreg(addr,2);
    set_debugreg(addr,3);
    printk("set data break:%lx,rsp:%lx\n",addr,read_rsp()); 
}
static err_t test_recv(void* arg, struct tcp_pcb* pcb, struct pbuf* p, err_t err)
{
    //printk("test_recv:%lx,buf:%lx\n",pcb,p);
    if (!pcb||!p) return 0;
    //dump_mem(p->payload, p->len);
    char *resp = "HTTP/1.1 200 OK\nContent-Length:5\nContent_Type:text/html\n\nhello";
    tcp_write(pcb, resp,strlen(resp),TCP_WRITE_FLAG_COPY);
    tcp_output(pcb);
    return 0;
}
static void test_conn_err(void *arg, signed char s)
{
    printk("conn_err\n");
}


static err_t test_accept(void *arg, struct tcp_pcb *pcb, err_t err)
{
    printk("test_accept：%d\n",err);
    tcp_arg(pcb, mem_calloc(4, 1));
    tcp_err(pcb, test_conn_err);
    tcp_recv(pcb, test_recv);
    return ERR_OK;

}
void test_tcp(lua_State *L)
{
    pcb = tcp_new();
    printk("pcb:%p\n",pcb);
    err_t ret = tcp_bind(pcb, IP_ADDR_ANY, 8080);
    printk("ret=%d\n",ret);
    pcb = tcp_listen(pcb);
    tcp_accept(pcb, test_accept);
    printk("accept,pcb:%p\n",pcb);

}
static void test_nsec (u64 nownsec,void *p)
{
    printk("test_nsec:%ld\n",nownsec/1000000000UL);
    //lua_getglobal(L,"cb1");
//    lua_pushstring(L,"1111");
  //  lua_call(L, 1, 1);
    struct tcp_pcb *cpcb;

    for(cpcb = tcp_active_pcbs;cpcb != NULL; cpcb = cpcb->next) {
         printk("acive pcb:%p\n",cpcb);
    }
 
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
//    char buf[4096];
    if(!isbp)return 0;

    printk("test_init,rsp:%lx,except_count:%d\n",read_rsp(),this_cpu_read(except_count));
    //memset(buf,0x5a,4096);
    //dump_mem(buf,0x100);
    set_data_break( (ulong)this_cpu_ptr(&except_count)-8);
    //set_data_break((ulong)buf);
    //printk("current:%s, stack:%lx,size:%lx,buf:%lx\n",current,current->stack_addr,current->stack_size,buf);
    set_timeout_nsec_poll(1000000000,test_nsec,(char *)10);
    set_timeout_nsec_poll(1500000000,test_nsec2,(char *)10);
    lua_State *L = getL();

     //LOAD_LUA_FILE(L,test);
    //LOAD_LUA_FILE(L,dump);
    test_tcp(L);
    do_lua_file(L, "test2");
    //DO_LUA_FILE(L,test2);

//    lua_close(L);
    //dump_mem(buf,0x100);
    //set_timeout_nsec(1000000000,test_nsec,(char *)10);
    //set_timeout_nsec(1500000000,test_nsec2,(char *)10);
    //dump_mem(buf,0x100);
    return 0;
}


main_initcall_sync(test_init);

