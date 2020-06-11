#include <yaos/types.h>
#include <drivers/virtio.h>
#include <yaoscall/malloc.h>
#include <yaoscall/page.h>
#include "virtio_net.h"
#include <net/ether.h>
#include <asm/apic.h>
#include <errno.h>
#include <yaos/queue.h>
#include <yaos/assert.h>
#include <string.h>
#include <yaos/time.h>
#include <yaos/kernel.h>
#include <yaos/msi.h>
#include <yaos/smp.h>
#include <api/net/ip.h>
#include <api/net/udp.h>

#define VIRTIO_NET_SUBID 0x1000
#define BUF_PACKETS             64
/* Maximum size of a packet */
#define MAX_PACK_SIZE           ETH_MAX_PACK_SIZE
/* Buffer size needed for the payload of BUF_PACKETS */
#define PACKET_BUF_SZ           (BUF_PACKETS * MAX_PACK_SIZE)
enum queue { RX_Q, TX_Q, CTRL_Q };

struct packet {
    int idx;
    struct virtio_net_hdr *vhdr;
    phys_bytes phdr;
    char *vdata;
    phys_bytes pdata;
    size_t len;
     STAILQ_ENTRY(packet) next;
};

static char *data_vir;
static phys_bytes data_phys;
static struct virtio_net_hdr *hdrs_vir;
static phys_bytes hdrs_phys;
static struct packet *packets;
static int in_rx;

/* Packets on this list can be given to the host */
static STAILQ_HEAD(free_list, packet) free_list;

/* Packets on this list are to be given to inet */
static STAILQ_HEAD(recv_list, packet) recv_list;

/* Various state data */
static eth_stat_t virtio_net_stats;

static const char *const name = "virtio-net";
static struct virtio_net pci_vn;
static void virtio_net_check_queues(void);
static void virtio_net_refill_rx_queue(void);
static void init_test_packet(char *destip,char *destmac);
static void check_packet(unsigned char *p,size_t size);
char arr[100] = {0xc0, 0xa8, 0xd1, 0x80, 0xc0, 0xa8, 0xd1, 0x01, 0x00, 0x0a, 0x00, 0x11, 0x13, 0x88, 0x13, 0x88, 0x00, 0x0a, 0x00, 0x00, 0x61, 0x66};
char tp[76]={0x33,0x33,0,0,0,0xfb,0x90,0x2b,0x34,0x57,0x30,0xa7,0x86,0xdd,0x60,0x07,0x5e,0xe1,0x0,0x16,0x11,0x01,0xfe,0x80,0,0,0,0,0,0,0x92,0x2b,0x34,0xff,0xfe,0x57,0x30,0xa7,0xff,0x02,0,0,0,0,0,0,0,0,0,0,0,0,0,0xfb,0x80,0xb2,0x11,0x5c,0x0,0x16,0x84,0x06,0x68,0x65,0x6c,0x6c,0x6f,0x20,0x69,0x27,0x6d,0x20,0x68,0x65,0x72,0x65};

char test_packet[1024];
char host_mac[6];
char host_ipv6[16];
struct udp_packet * ptest_udp;
struct udp6_packet *ptest_udp6;
u32 check_sum(u32 sum,void *buf, int len)
{
    int num = 0;
    uchar *p = (uchar *)buf; 
    if ((NULL == p) || (0==len)) return sum;
    while (len > 1) {
        sum += ((ushort)p[num]<<8&0xff00)|((ushort)p[num+1]&0xff);
        len -= 2;
        num += 2;
    }
 
    if (len>0) {
        sum += ((ushort)p[num]<<8)&0xffff;
        num++;
    }
 
    while (sum >> 16) {
        sum = (sum >> 16) + (sum & 0xffff);
    }
    return sum; 
}
static void irq_handler_receive(int n)
{
    uchar isr =
    virtio_conf_readb(to_virtio_dev_t(&pci_vn), VIRTIO_PCI_ISR);
    ack_lapic_irq();
    virtio_queue_disable_intr(virtio_get_queue(to_virtio_dev_t(&pci_vn), RX_Q));
    printk("****virtio_net_irq_handler**** irq:%d,isr:%d cpu:%d\n", n,isr,smp_processor_id());
    virtio_net_check_queues();
}
static void irq_handler_send(int n)
{
    uchar isr =
    virtio_conf_readb(to_virtio_dev_t(&pci_vn), VIRTIO_PCI_ISR);
    ack_lapic_irq();
    virtio_queue_disable_intr(virtio_get_queue(to_virtio_dev_t(&pci_vn), TX_Q));

    printk("****virtio_net_irq_handler**** irq:%d,isr:%d cpu:%d\n", n,isr,smp_processor_id());
    virtio_net_check_queues();
}
static void irq_handler_control(int n)
{
    uchar isr =
    virtio_conf_readb(to_virtio_dev_t(&pci_vn), VIRTIO_PCI_ISR);
    ack_lapic_irq();
    virtio_queue_disable_intr(virtio_get_queue(to_virtio_dev_t(&pci_vn), CTRL_Q));
    printk("****virtio_net_irq_handler**** irq:%d,isr:%d cpu:%d\n", n,isr,smp_processor_id());
    virtio_net_check_queues();
}

void virtio_net_read_config(struct virtio_net *p)
{
    virtio_dev_t pvirtio = to_virtio_dev_t(p);
    u32 offset = virtio_pci_config_offset(to_virtio_dev_t(p));

    virtio_conf_read(to_virtio_dev_t(p), offset, &p->_config,
                     sizeof(p->_config));
    if (virtio_get_guest_feature_bit(to_virtio_dev_t(p), VIRTIO_NET_F_MAC)) {
        printk("The mac addr of the device is: %x:%x:%x:%x:%x:%x\n",
               (u32) p->_config.mac[0],
               (u32) p->_config.mac[1],
               (u32) p->_config.mac[2],
               (u32) p->_config.mac[3],
               (u32) p->_config.mac[4], (u32) p->_config.mac[5]);

    }
    p->_mergeable_bufs =
        virtio_get_guest_feature_bit(pvirtio, VIRTIO_NET_F_MRG_RXBUF);
    p->_status = virtio_get_guest_feature_bit(pvirtio, VIRTIO_NET_F_STATUS);
    p->_tso_ecn = virtio_get_guest_feature_bit(pvirtio, VIRTIO_NET_F_GUEST_ECN);
    p->_host_tso_ecn =
        virtio_get_guest_feature_bit(pvirtio, VIRTIO_NET_F_HOST_ECN);
    p->_csum = virtio_get_guest_feature_bit(pvirtio, VIRTIO_NET_F_CSUM);
    p->_guest_csum =
        virtio_get_guest_feature_bit(pvirtio, VIRTIO_NET_F_GUEST_CSUM);
    p->_guest_tso4 =
        virtio_get_guest_feature_bit(pvirtio, VIRTIO_NET_F_GUEST_TSO4);
    p->_host_tso4 =
        virtio_get_guest_feature_bit(pvirtio, VIRTIO_NET_F_HOST_TSO4);
    p->_guest_ufo =
        virtio_get_guest_feature_bit(pvirtio, VIRTIO_NET_F_GUEST_UFO);
    p->_ctrl_vq = virtio_get_guest_feature_bit(pvirtio, VIRTIO_NET_F_CTRL_VQ);
    pci_d("Features: %s=%d,%s=%d\n", "Status", p->_status, "TSO_ECN",
          p->_tso_ecn);
    pci_d("Features: %s=%d,%s=%d\n", "Host TSO ECN", p->_host_tso_ecn, "CSUM",
          p->_csum);
    pci_d("Features: %s=%d,%s=%d\n", "Guest_csum", p->_guest_csum, "guest tso4",
          p->_guest_tso4);
    pci_d("Features: %s=%d,%s=%d\n", "host tso4", p->_host_tso4, "ctrl_vq",
          p->_ctrl_vq);
}
struct net_req {
    struct net_hdr_mrg_rxbuf mhdr;
    uchar status;
} req;

static void virtio_net_check_queues(void)
{
    struct packet *p;
    size_t len;

    /* Put the received packets into the recv list */
    while (virtio_from_queue(to_virtio_dev_t(&pci_vn), RX_Q, (void **)&p, &len)
           == 0) {
        pci_d("virtio_from_RX_queue:%lx,len:%x\n", p, len);
        //continue;
        p->len = len;
       // pci_d("vhdr:%lx,phdr:%lx,vdata:%lx,pdata:%lx,len:%d\n",
       //       p->vhdr, p->phdr, p->vdata, p->pdata, p->len);
//dump_mem(p->vdata,len);
        struct virtio_net_hdr {
            u8 flags;
            u8 gso_type;
            u16 hdr_len;        /* Ethernet + IP + tcp/udp hdrs */
            u16 gso_size;       /* Bytes to append to hdr_len per frame */
            u16 csum_start;     /* Position to start checksumming from */
            u16 csum_offset;    /* Offset after that to place checksum */
        };
/*
        pci_d
            ("flags:%x,gso_type:%x,hdr_len:%x,gso_size:%x,csum_start:%x,csum_offset:%x\n",
             p->vhdr->flags, p->vhdr->gso_type, p->vhdr->hdr_len,
             p->vhdr->gso_size, p->vhdr->csum_start, p->vhdr->csum_offset);
        ulong *pl = (ulong *) p->vdata;

        pci_d("%lx,%lx,%lx,%lx,%lx,%lx,%lx\n", pl[0], pl[1], pl[2], pl[3],
              pl[4], pl[5], pl[6]);
        */
        u16 *pw = (u16 *) p->vdata+5;
        if(pw[6]==0x2600)continue;
        //pci_d("%lx,%lx,%lx\n", pw[5], pw[6], pw[7]);
        struct udp_packet *pudp = (struct udp_packet *)(p->vdata+10);
        struct udp6_packet *pudp6 = (struct udp6_packet *)(p->vdata+10);

        uchar *pc = (uchar *) p->vdata+10;
        memcpy(host_mac,&pc[6],6);
        pci_d("To:%x:%x:%x:%x:%x:%x,From:%x:%x:%x:%x:%x:%x: len:%x,type:%x,prot:%x,port:%x,len:%x\n",
              pc[0], pc[1], pc[2], pc[3], pc[4], pc[5], pc[6], pc[7], pc[8],
              pc[9], pc[10], pc[11],pw[9],pw[6],pc[20],pudp->udp.dest,ntohs(pudp->ip.ip_len));
        
        //if (pw[6]==0xdd86)pci_d("ipv6\n");
        //if (pc[20]==0x11)pci_d("udp\n"); 
        if (pw[6]==0x0008 && pudp->ip.ip_p==0x11 && pudp->udp.dest==0x5c11) {
            dump_mem(pudp,ntohs(pudp->ip.ip_len)+14);
            ushort oldipsum = pudp->ip.ip_sum;
            ushort oldudpsum = pudp->udp.check;
            pudp->ip.ip_sum = 0;
            pudp->udp.check = 0;
            ip_set_checksum(&pudp->ip);
            ip_set_udp_checksum(&pudp->ip);
            printk("oldip:%x,newip:%x,oldudp:%x,newudp:%x\n",oldipsum,pudp->ip.ip_sum,oldudpsum,pudp->udp.check);
        } 
        if (pw[6]==0xdd86 && pc[20]==0x11){
            ushort oldudpsum = pudp6->udp.check;
            pudp->udp.check = 0;
            ip_set_udp6_checksum(&pudp6->ip6);
            printk("oldudpv6:%x,%x,newudp:%x,len:%x,%x\n",oldudpsum,pw[30],pudp6->udp.check,ntohs(pudp6->udp.len),len-64);

            ushort oldsum = pw[30];
            pw[30]=0;
            u32 sum = 0;
            sum+=len-64;
            sum = check_sum(sum,&pw[11],32);
            sum += 0x11;
            memcpy(host_ipv6,&pw[11],16);
            ushort calsum = (ushort)check_sum(sum,&pw[27],len-64);
            pci_d("oldsum:%x,calsum:%x\n",oldsum,~calsum&0xffff);
            //dump_mem(pudp,ntohs(pudp->ip.ip_len)+14);
            //init_test_packet(&pc[22],&pc[6]);
        }
        STAILQ_INSERT_TAIL(&recv_list, p, next);
        in_rx--;
        virtio_net_stats.ets_packetR++;
           
    }
void virtio_net_refill_rx_queue(void);
    virtio_net_refill_rx_queue();
    /*
     * Packets from the TX queue just indicated they are free to
     * be reused now. inet already knows about them as being sent.
     */
    while (virtio_from_queue(to_virtio_dev_t(&pci_vn), TX_Q, (void **)&p, NULL)
           == 0) {
        pci_d("virtio_from_TX_queue:%lx,%lx,len:%x,ret:%d\n", p,&req, len,req.status);
//struct udp_packet *p = (struct udp_packet *)ptest_udp;
//dump_mem(p,76);
/*
        ulong *pl = (ulong *) p;

        pci_d("%lx,%lx,%lx,%lx,%lx,%lx,%lx\n", pl[0], pl[1], pl[2], pl[3],
              pl[4], pl[5], pl[6]);
        memset(p->vhdr, 0, sizeof(*p->vhdr));
        memset(p->vdata, 0, MAX_PACK_SIZE);
        STAILQ_INSERT_HEAD(&free_list, p, next);
        virtio_net_stats.ets_packetT++;
*/
    }
}
void virtio_net_send_packet(void *buf,size_t size)
{
    struct addr_size phys[3];
    struct net_hdr_mrg_rxbuf mhdr = {.hdr={.gso_type=0},.num_buffers=2};
    memset(&mhdr,0,sizeof(mhdr));
    mhdr.num_buffers=0;
    mhdr.hdr.csum_start=0x22;
    mhdr.hdr.csum_offset=0x10;
    mhdr.hdr.flags = 1;
    phys[0].vp_addr = V2P(&mhdr);
    phys[0].vp_size = sizeof(mhdr.hdr);
    phys[0].vp_flags = VRING_DESC_F_READ;
    phys[1].vp_addr = V2P(buf);
    phys[1].vp_size = size;
    phys[1].vp_flags = VRING_DESC_F_READ;
    virtio_to_queue(to_virtio_dev_t(&pci_vn), TX_Q, phys, 2, &req);

    
}
__used static void init_udp_packet(char *destip,char *destmac)
{
    struct udp_packet *p = ptest_udp;
    eth_set_hdr(&p->eth, destmac, pci_vn._config.mac,ETH_IP_PROTO);
    char myip[4]={192,168,122,98};
    ip_init_hdr(&p->ip,destip,myip,0x11);
    p->udp.source = htons(4444);
    p->udp.dest = htons(4444);
    udp_set_data(p,"Hello Yaos!", 12);
    //printk("ip.len:%d\n",ntohs(p->ip.ip_len));
    dump_mem(p,ntohs(p->ip.ip_len)+sizeof(p->eth));
    
}
__used static void init_udp6_packet(char *destip,char *destmac)
{
    struct udp6_packet *p = ptest_udp6;
    eth_set_hdr(&p->eth, destmac, pci_vn._config.mac,ETH_P_IPV6);
    char myip[16]={0,0,0,0,0,0,0,0,0,0,0xff,0xff,192,168,122,98};
    ip6_init_hdr(&p->ip6,destip,myip,0x11);
    p->udp.source = htons(4444);
    p->udp.dest = htons(4444);
    udp6_set_data(p,"Hello Yaos! From IPV6", 22);
    //printk("ip.len:%d\n",ntohs(p->ip.ip_len));
    dump_mem(p,ntohs(p->udp.len)+sizeof(p->ip6)+sizeof(p->eth));

}

__used static void init_test_packet(char *destip,char *destmac)
{
    uchar len=51-32;
    char *p = &test_packet[22];
    struct virtio_net *dev = &pci_vn;
    p[0]=0xfe;
    p[1]=0x80;
    p[2]=p[3]=p[4]=p[5]=p[6]=p[7]=0;
    p[8]=(dev->_config.mac[0]-2)&0xff; 
    p[9]=dev->_config.mac[1];
    p[10]=dev->_config.mac[2];
    p[11]=0xff;
    p[12]=0xfe;
    p[13]=dev->_config.mac[3];
    p[14]=dev->_config.mac[4];
    p[15]=dev->_config.mac[5];
    char myip[16]={0,0,0,0,0,0,0,0,0,0,0xff,0xff,192,168,122,98};
    memcpy(&p[8],myip,16);
    char *dip = (char *)destip;
    for(int i=0;i<16;i++)p[16+i]=*dip++;
    p[32]=0x11; //port 4444
    p[33]=0x5c;
    p[34]=0x11;
    p[35]=0x5c;  
   
    //p36-37 length
    p[36]=0;
    p[37]=len;
    //p38-39 sum
    p[38]=0;
    p[39]=0;
    p[40]='H';
    p[41]='E';
    p[42]='L';
    p[43]='L';
    p[44]='O';
    p[45]=' ';
    p[46]='Y';
    p[47]='A';
    p[48]='O';
    p[49]='S';
    p[50]=0;
    //char tomac[6]={0x90,0x2b,0x34,0x57,0x30,0xa7};
    //char toip[16]={0xfe,0x80,0,0,0,0,0,0,0x92,0x2b,0x34,0xff,0xfe,0x57,0x30,0xa7};
    //memcpy(test_packet,tomac,6);
    //memcpy(&test_packet[38],toip,16);
    memcpy(test_packet,destmac,6);
    memcpy(&test_packet[6],dev->_config.mac,6);
    p=&test_packet[12];
    *p++ = 0x86;
    *p++ = 0xdd;
    *p++ = 0x60;
    *p++ = 0x02;
    *p++ = 0x53;
    *p++ = 0x8d;
    *p++ = 0;
    *p++ = len;
    *p++ = 0x11;
    *p++ = 0xff;
    u32 sum = 0;
    sum+=len;
    sum = check_sum(sum,&test_packet[22],32);
    sum += 0x11;
    ushort calsum = ~(ushort)check_sum(sum,&test_packet[54],len);
    pci_d("test packet calsum:%x\n",calsum);
    test_packet[60]=(calsum>>8)&0xff;
    test_packet[61]=(calsum)&0xff;
    dump_mem(test_packet,51+22);
    //virtio_net_send_packet(test_packet,51+22);
}
static void virtio_net_refill_rx_queue(void)
{
    struct addr_size phys[2];
    struct packet *p;

    while ((in_rx < BUF_PACKETS / 2) && !STAILQ_EMPTY(&free_list)) {
        /* peek */
        p = STAILQ_FIRST(&free_list);
        /* remove */
        STAILQ_REMOVE_HEAD(&free_list, next);

        //phys[0].vp_addr = p->phdr;
        //phys[0].vp_size = sizeof(struct virtio_net_hdr);
        //phys[0].vp_flags = VRING_DESC_F_WRITE;
        phys[0].vp_addr = p->pdata;
        phys[0].vp_size = MAX_PACK_SIZE;
        phys[0].vp_flags = VRING_DESC_F_WRITE;
        //virtio_to_queue(to_virtio_dev_t(&pci_vn), RX_Q, phys, 2, p);

        virtio_to_queue(to_virtio_dev_t(&pci_vn), RX_Q, phys, 1, p);
        in_rx++;
    }

    if (in_rx == 0 && STAILQ_EMPTY(&free_list)) {
        pci_w(("warning: rx queue underflow!"));
        virtio_net_stats.ets_fifoUnder++;
    }
}

static void virtio_net_init_queues(void)
{
    int i;

    STAILQ_INIT(&free_list);
    STAILQ_INIT(&recv_list);

    for (i = 0; i < BUF_PACKETS; i++) {
        packets[i].idx = i;
        packets[i].vhdr = &hdrs_vir[i];
        packets[i].phdr = hdrs_phys + i * sizeof(hdrs_vir[i]);
        packets[i].vdata = data_vir + i * MAX_PACK_SIZE;
        packets[i].pdata = data_phys + i * MAX_PACK_SIZE;
        STAILQ_INSERT_HEAD(&free_list, &packets[i], next);
    }
}

static int virtio_net_alloc_bufs(void)
{
    data_vir = yaos_malloc(PACKET_BUF_SZ);
    data_phys = (phys_bytes) yaos_v2p(data_vir);
    pci_d("data_vir:%lx\n", data_vir);
    yaos_mfree(data_vir);
    data_vir = yaos_malloc(PACKET_BUF_SZ);
    data_phys = (phys_bytes) yaos_v2p(data_vir);
    pci_d("data_vir:%lx\n", data_vir);

    if (!data_vir)
        return ENOMEM;
    hdrs_vir = yaos_malloc(BUF_PACKETS * sizeof(hdrs_vir[0]));
    hdrs_phys = (phys_bytes) yaos_v2p(hdrs_vir);

    if (!hdrs_vir) {
        yaos_mfree(data_vir);
        return ENOMEM;
    }

    packets = yaos_malloc(BUF_PACKETS * sizeof(packets[0]));

    if (!packets) {
        yaos_mfree(data_vir);
        yaos_mfree(hdrs_vir);
        return ENOMEM;
    }

    memset(data_vir, 0, PACKET_BUF_SZ);
    memset(hdrs_vir, 0, BUF_PACKETS * sizeof(hdrs_vir[0]));
    memset(packets, 0, BUF_PACKETS * sizeof(packets[0]));

    return OK;
}
__used static void vn_timeout(u64 nowmsec,void *param)
{
    set_timeout_nsec(10000000000, vn_timeout,param);

    pci_d("vntimeout:now msec:%d\n", nowmsec/1000000UL);
    //virtio_net_check_queues();
 //char tomac[6]={0x90,0x2b,0x34,0x57,0x30,0xa7};
// char tomac[6]={0x33,0x33,0x0,0x0,0x0,0xfb};
//char toip[16]={0xfe,0x80,0,0,0,0,0,0,0x92,0x2b,0x34,0xff,0xfe,0x57,0x30,0xa7};
//char toip[16]={0xff,0x02,0,0,0,0,0,0, 0,0,0,0,0,0,0,0xfb};
// init_test_packet(host_ipv6,host_mac);
    init_udp6_packet(host_ipv6,host_mac);
    virtio_net_send_packet(ptest_udp6,ntohs(ptest_udp6->udp.len)+sizeof(ptest_udp6->eth)+sizeof(ptest_udp6->ip6));
//check_packet(test_packet,73);
// dump_mem(test_packet,73);
//char t[76]={0x33,0x33,0,0,0,0xfb,0x90,0x2b,0x34,0x57,0x30,0xa7,0x86,0xdd,0x60,0x07,0x5e,0xe1,0x0,0x16,0x11,0x01,0xfe,0x80,0,0,0,0,0,0,0x92,0x2b,0x34,0xff,0xfe,0x57,0x30,0xa7,0xff,0x02,0,0,0,0,0,0,0,0,0,0,0,0,0,0xfb,0x80,0xb2,0x11,0x5c,0x0,0x16,0x84,0x06,0x68,0x65,0x6c,0x6c,0x6f,0x20,0x69,0x27,0x6d,0x20,0x68,0x65,0x72,0x65};
    char toip4[4]={192,168,122,1};
    init_udp_packet(toip4,host_mac);
    struct udp_packet *p = ptest_udp;
    virtio_net_send_packet(p,ntohs(p->ip.ip_len)+sizeof(p->eth));

//virtio_net_send_packet(t,76);
 //dump_mem(t,76);
}
static void check_packet(unsigned char *p,size_t size){
    u32 sum = 0;
    ushort len = 0;
    ushort oldsum = *((ushort *)&p[60]);
    *((ushort *)&p[60])=0;
    len = p[59]+((ushort)p[58]<<8);
    sum+=len;
    sum = check_sum(sum,&p[22],32);
    sum += 0x11;
    ushort calsum = ~(ushort)check_sum(sum,&p[54],len);
    pci_d("len:%x,%x,oldsum:%x,calsum:%x\n",len,size,oldsum,calsum);
    *((ushort *)&p[60])=oldsum; 
}
int init_virtio_net()
{
    virtio_dev_t dev = to_virtio_dev_t(&pci_vn);
    
    if (virtio_setup_device(dev, VIRTIO_NET_SUBID, 1)) {
        virtio_net_read_config(&pci_vn);
        /* virtio-net has at least 2 queues */

        int queues = 2;
        irq_handler_t handlearr[3]={[RX_Q]=irq_handler_receive,[TX_Q]=irq_handler_send,
               [CTRL_Q]=irq_handler_control};
      
        if (pci_vn._ctrl_vq) {
            queues++;
        }
        pci_dump_config(to_pci_device_t(&pci_vn));
        msi_register_arr(to_pci_device_t(&pci_vn), 0, queues, handlearr);
        int ret;
        if ((ret=virtio_net_alloc_bufs()) != OK)
            panic("%s: Buffer allocation failed", name);
        pci_d("virtio_net_alloc_bufs:%d\n", ret);
pci_d("test sum:%x,%x\n",~check_sum(0,arr,22));
check_packet(tp,76);
        virtio_net_init_queues();

        virtio_device_ready(to_virtio_dev_t(&pci_vn));
ptest_udp = (struct udp_packet *)(alloc_kheap_4k(0x1000));
ptest_udp6 = (struct udp6_packet *)(alloc_kheap_4k(0x1000));


virtio_net_refill_rx_queue();

        set_timeout_nsec(1000000000UL, vn_timeout, &pci_vn);
        sti();

    }
    return OK;
}
