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
#include "lwip/init.h"
#include "lwip/ip.h"
#include "lwip/ip_addr.h"
#include "lwip/priv/tcp_priv.h"
#include "lwip/udp.h"
#include "lwip/inet_chksum.h"

static struct pbuf *
test_udp_create_test_packet(u16_t length, u16_t port, u32_t dst_addr)
{
  struct udp_hdr *uh;
  struct ip_hdr *ih;
  struct pbuf *p;
  const u8_t test_data[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf};

  p = pbuf_alloc(PBUF_TRANSPORT, length, PBUF_POOL);
  if (p == NULL) {
    return NULL;
  }
  pbuf_take(p, test_data, length);

  /* add UDP header */
  pbuf_add_header(p, sizeof(struct udp_hdr));
  uh = (struct udp_hdr *)p->payload;
  uh->chksum = 0;
  uh->dest = uh->src = lwip_htons(port);
  uh->len = lwip_htons(p->tot_len);
  /* add IPv4 header */
  pbuf_add_header(p, sizeof(struct ip_hdr));
  ih = (struct ip_hdr *)p->payload;
  memset(ih, 0, sizeof(*ih));
  ih->dest.addr = dst_addr;
  ih->_len = lwip_htons(p->tot_len);
  ih->_ttl = 32;
  ih->_proto = IP_PROTO_UDP;
  IPH_VHL_SET(ih, 4, sizeof(struct ip_hdr) / 4);
  IPH_CHKSUM_SET(ih, inet_chksum(ih, sizeof(struct ip_hdr)));
  return p;
}
int test_udp()
{
    int send_pbuf(struct pbuf *p);
    int ret;
    static ip4_addr_t addr;
    IP4_ADDR(&addr, 192,168,122,1);
    struct pbuf *p = test_udp_create_test_packet(16, 4444, addr.addr);
    printk("p:%lx\n",p);
    if (!p)return ENOMEM;
    for (int i=0;i<2;i++)if ((ret=send_pbuf(p))!=0)return ret;
    pbuf_free(p);
    return 0;
}
struct udp_pcb *pudp_pcb = NULL;
int test_udp2()
{
    if (pudp_pcb==NULL) pudp_pcb = udp_new();
    struct pbuf *p;
    const u8_t test_data[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf};
    static long idx = 0;
 
    ip_addr_t addr;
    //addr.type = IPADDR_TYPE_V4;
    IP4_ADDR(&addr, 192,168,122,1);

    err_t ret;
    int i;
    for ( i=0;i<100;i++){
        p = pbuf_alloc(PBUF_TRANSPORT, 16, PBUF_RAM);
        if (p == NULL) {
          ret = -9;
          break;
        }
        sprintf((char *)test_data,"%ld\n",idx++); 
        pbuf_take(p, test_data, 16);
        ret = udp_sendto(pudp_pcb, p, &addr, 4444);
        pbuf_free(p);
    }

    if(ret)printk("udp_sendto:%d,cnt:%d\n",ret,i);
    return ret;  
}
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
__used static void test_nsec2 (u64 nownsec,void *p)
{
    printk("test_nsec2:%ld\n",nownsec/1000000000UL);
    test_udp2();
    set_timeout_nsec_poll(500000000,test_nsec2,(char *)10);

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
    //set_data_break( (ulong)this_cpu_ptr(&except_count)-8);
    //set_data_break((ulong)buf);
    //printk("current:%s, stack:%lx,size:%lx,buf:%lx\n",current,current->stack_addr,current->stack_size,buf);
    set_timeout_nsec_poll(1000000000,test_nsec,(char *)10);
    //set_timeout_nsec_poll(1500000000,test_nsec2,(char *)10);
    lua_State *L = getL();


     //LOAD_LUA_FILE(L,test);
    //LOAD_LUA_FILE(L,dump);
    test_tcp(L);
    void free_phy_page_small(ulong addr, ulong size);
    free_phy_page_small(0x300000,0x1000);
    free_phy_page_small(0x300000,0x1000);
    printk("free done\n");
#if 0
    ulong alloc_small_phy_page_safe();
    ulong alloc_phy_page();
    void free_phy_one_page();
    ulong arr[150];
    for (int i=0;i<150;i++) {
        ulong page = alloc_phy_page();
        arr[i]=page;
        printk("ulong alloc_small_phy_page_safe:%lx\n",page);
        //free_phy_one_page(page);
    }
    for (int i=0;i<150;i++)free_phy_one_page(arr[i]);
#endif
    //do_lua_file(L, "test2");
    for(int i=0;i<10;i++)do_lua_file(getL(), "test3");

//    lua_close(L);
    //dump_mem(buf,0x100);
    //set_timeout_nsec(1000000000,test_nsec,(char *)10);
    //set_timeout_nsec_poll(1500000000,test_nsec2,(char *)10);
    //dump_mem(buf,0x100);
    return 0;
}


main_initcall_sync(test_init);

