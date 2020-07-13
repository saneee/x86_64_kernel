#include <yaos/types.h>
#include <yaos/module.h>
#include <yaos/printk.h>
#include <yaos/init.h>
#include <drivers/virtio.h>
#include <api/errno.h>
#include <string.h>
#include "virtio_net.h"
#include "lwip/init.h"
#include "lwip/netif.h"
#include "lwip/dns.h"
#include "netif/etharp.h"
#if LWIP_IPV6
#include "lwip/ethip6.h"
#include "lwip/nd6.h"
#endif

#include "netif/ethernet.h"
#include "lwip/ip.h"
#include "lwip/ethip6.h"
#include "lwip/apps/httpd.h"
DECLARE_MODULE(virtio_net_drivers, 0, main);
void init_virtio_net();
struct netif virtio_netif;
static err_t virtio_netif_init(struct netif *pif);
extern err_t virtio_net_send_packet(void *buf,size_t size,void *data);
extern err_t virtio_net_send_packets(struct iovec *pvec,int nr,void *data);
static struct virtio_net *dev_vn = 0;
#define MAX_VECS 10
static void lwip_tx_done(void *data)
{
    //printk("lwip_tx_done:%lx,%d\n",data, ((struct pbuf *)data)->ref);
    pbuf_free((struct pbuf *)data);    
}
static err_t lwip_tx_func(struct netif *netif, struct pbuf *p)
{
    pbuf_ref(p);
    struct pbuf *q;
    //err_t  ret;
    struct iovec vecs[MAX_VECS];
    int nr_vec = 0;
    for (q = p; q != NULL; q = q->next) {
        //printk("q->payload:%016lx,size:%x\n",q->payload,q->len);
        ulong pageend = ((ulong)q->payload+q->len)&~0xfff;
        if (((ulong)q->payload & ~0xfff) == pageend ) {
            //if ((ret = virtio_net_send_packet(q->payload, q->len))!=0) return ret;
            vecs[nr_vec].iov_base = q->payload;
            vecs[nr_vec].iov_len = q->len;
            nr_vec++;  
            ASSERT(nr_vec<MAX_VECS);
        } else {
            //printk("try to send not same page\n");
            //ASSERT(0);
            vecs[nr_vec].iov_base = q->payload;
            vecs[nr_vec].iov_len = pageend - (ulong)q->payload;
            nr_vec++;
            vecs[nr_vec].iov_base = (void *)pageend; 
            vecs[nr_vec].iov_len = ((ulong)q->payload+q->len)&0xfff;
            nr_vec++;

        }
        //dump_mem(q->payload, q->len);
    }
   
    return (nr_vec && virtio_net_send_packets(vecs, nr_vec, p) ) || ERR_OK;
}
int send_pbuf(struct pbuf *p)
{
    return lwip_tx_func(&virtio_netif,p);
}
static int receiver(char *buf, size_t len, void *param)
{
    struct netif *netif = (struct netif*) param;
    struct pbuf *p, *q;
    err_t err;
    //dump_mem(buf,len);
    LWIP_ASSERT("pkt too big", len <= 0xFFFF);
    p = pbuf_alloc(PBUF_RAW, (u16_t)len, PBUF_POOL);
    LWIP_ASSERT("alloc failed", p);
    for(q = p; q != NULL; q = q->next) {
        MEMCPY(q->payload, buf, q->len);
       buf += q->len;
    }
    err = netif->input(p, netif);
    //err = 0;
    //printk("netif->input:%016lx,err:%d\n",netif->input,err);
    if (err != ERR_OK) {
        pbuf_free(p);
    }
    return 0; 
}
static void tick_handle(u64 nownsec,void *p)
{
    set_timeout_nsec_poll(1000000,tick_handle,p);
    extern void sys_check_timeouts();
    sys_check_timeouts();
}
__used static int setup_driver(struct virtio_net * dev)
{
    dev_vn = dev;
    dev_vn->tx_done = lwip_tx_done;
    return 0;
}
__used static int main_setup(bool isbp)
{
    if (!isbp || !dev_vn)return 0;

    struct virtio_net *dev = dev_vn;
    ip4_addr_t addr;
    ip4_addr_t netmask;
    ip4_addr_t gw;

    virtio_netif.hwaddr[0] = dev->_config.mac[0];
    virtio_netif.hwaddr[1] = dev->_config.mac[1];
    virtio_netif.hwaddr[2] = dev->_config.mac[2];
    virtio_netif.hwaddr[3] = dev->_config.mac[3];
    virtio_netif.hwaddr[4] = dev->_config.mac[4];
    virtio_netif.hwaddr[5] = dev->_config.mac[5];
    dev->receiver = receiver;
    IP4_ADDR(&addr, 192, 168, 122, 98);
    IP4_ADDR(&netmask, 255, 255, 255, 0);
    IP4_ADDR(&gw, 192, 168, 122, 1);

    netif_add(&virtio_netif, &addr, &netmask, &gw, NULL, virtio_netif_init, ethernet_input);
    netif_set_up(&virtio_netif);
    netif_set_link_up(&virtio_netif);
    netif_set_default(&virtio_netif);
    #if LWIP_IPV6
    nd6_tmr(); /* tick nd to join multicast groups */
    #endif
    dns_setserver(0, &virtio_netif.gw);

    httpd_init();
    set_timeout_nsec_poll(1000000,tick_handle,&virtio_netif);


    return 0;
}
static err_t virtio_netif_init(struct netif *pif)
{
    pif->name[0] = 'v';
    pif->name[1] = 'n';
    pif->output = etharp_output;
    pif->linkoutput = lwip_tx_func;
    pif->mtu = 1500;
    pif->hwaddr_len = 6;
    pif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_IGMP;


#if LWIP_IPV6
    pif->output_ip6 = ethip6_output;
    pif->ip6_autoconfig_enabled = 1;
    netif_create_ip6_linklocal_address(pif, 1);
    pif->flags |= NETIF_FLAG_MLD6;
#endif

    return ERR_OK;

}
__used __init static int init_lwip_netif()
{
    lwip_init();
    
    return MOD_ERR_OK;    
}

__used __init static int main(module_t m, ulong t, void *arg)
{
    modeventtype_t env = (modeventtype_t) t;
    printk("virtiomain env:%d,%d\n",env,MOD_BPDEVICE);
    if (env == MOD_BPLOAD) {
        printk("virtio_net_drivers, %lx,%lx,%lx\n", m, t, arg);
    } else if(env == MOD_BPDEVICE) {
        printk("init_virtio_net\n");
        init_lwip_netif();
        init_virtio_net(setup_driver, &virtio_netif);
    }
    return MOD_ERR_OK;          //MOD_ERR_NEXTLOOP;
}
main_initcall(main_setup);

