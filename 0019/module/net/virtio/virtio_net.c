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

static void virtio_net_check_queues(void)
{
    struct packet *p;
    size_t len;

    /* Put the received packets into the recv list */
    while (virtio_from_queue(to_virtio_dev_t(&pci_vn), RX_Q, (void **)&p, &len)
           == 0) {
        pci_d("virtio_from_queue:%lx,len:%x\n", p, len);
        p->len = len;
        pci_d("vhdr:%lx,phdr:%lx,vdata:%lx,pdata:%lx,len:%d\n",
              p->vhdr, p->phdr, p->vdata, p->pdata, p->len);
        struct virtio_net_hdr {
            u8 flags;
            u8 gso_type;
            u16 hdr_len;        /* Ethernet + IP + tcp/udp hdrs */
            u16 gso_size;       /* Bytes to append to hdr_len per frame */
            u16 csum_start;     /* Position to start checksumming from */
            u16 csum_offset;    /* Offset after that to place checksum */
        };

        pci_d
            ("flags:%x,gso_type:%x,hdr_len:%x,gso_size:%x,csum_start:%x,csum_offset:%x\n",
             p->vhdr->flags, p->vhdr->gso_type, p->vhdr->hdr_len,
             p->vhdr->gso_size, p->vhdr->csum_start, p->vhdr->csum_offset);
        ulong *pl = (ulong *) p->vdata;

        pci_d("%lx,%lx,%lx,%lx,%lx,%lx,%lx\n", pl[0], pl[1], pl[2], pl[3],
              pl[4], pl[5], pl[6]);
        u16 *pw = (u16 *) p->vdata;

        pci_d("%lx,%lx,%lx\n", pw[5], pw[6], pw[7]);
        uchar *pc = (uchar *) p->vdata+8;
        pci_d("To:%x:%x:%x:%x:%x:%x,From:%x:%x:%x:%x:%x:%x: len:%x,type:%x\n",
              pc[0], pc[1], pc[2], pc[3], pc[4], pc[5], pc[6], pc[7], pc[8],
              pc[9], pc[10], pc[11],pw[10],pw[11]);
        STAILQ_INSERT_TAIL(&recv_list, p, next);
        in_rx--;
        virtio_net_stats.ets_packetR++;
    }
    /*
     * Packets from the TX queue just indicated they are free to
     * be reused now. inet already knows about them as being sent.
     */
    while (virtio_from_queue(to_virtio_dev_t(&pci_vn), TX_Q, (void **)&p, NULL)
           == 0) {
        pci_d("virtio_from_queue:%lx,len:%x\n", p, len);
        ulong *pl = (ulong *) p;

        pci_d("%lx,%lx,%lx,%lx,%lx,%lx,%lx\n", pl[0], pl[1], pl[2], pl[3],
              pl[4], pl[5], pl[6]);
        memset(p->vhdr, 0, sizeof(*p->vhdr));
        memset(p->vdata, 0, MAX_PACK_SIZE);
        STAILQ_INSERT_HEAD(&free_list, p, next);
        virtio_net_stats.ets_packetT++;
    }
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

        phys[0].vp_addr = p->phdr;
        ASSERT(!(phys[0].vp_addr & 1));
        phys[0].vp_size = sizeof(struct virtio_net_hdr);

        phys[1].vp_addr = p->pdata;
        ASSERT(!(phys[1].vp_addr & 1));
        phys[1].vp_size = MAX_PACK_SIZE;

        /* RX queue needs write */
        phys[0].vp_addr |= 1;
        phys[1].vp_addr |= 1;

        virtio_to_queue(to_virtio_dev_t(&pci_vn), RX_Q, phys, 2, p);
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

static void vn_timeout(u64 nowmsec)
{
    set_timeout(5000, vn_timeout);
    pci_d("vntimeout:now msec:%d\n", nowmsec);
    virtio_net_check_queues();
}

int init_virtio_net()
{
    virtio_dev_t dev = to_virtio_dev_t(&pci_vn);

    if (virtio_setup_device(dev, VIRTIO_NET_SUBID, 1)) {
        virtio_net_read_config(&pci_vn);
        /* virtio-net has at least 2 queues */

        int queues = 2;

        if (pci_vn._ctrl_vq) {
            queues++;
        }
        int ret = virtio_alloc_queues(dev, queues);

        pci_d("virtio_alloc_queues ret:%d\n", ret);
        if (virtio_net_alloc_bufs() != OK)
            panic("%s: Buffer allocation failed", name);
        pci_d("virtio_net_alloc_bufs:%d\n", ret);

        virtio_net_init_queues();
        pci_d("virtio_net_init_queues:%d\n", ret);
        /* Add packets to the receive queue. */
        virtio_net_refill_rx_queue();
        pci_d("virtio_net_refill_rx_queue:%d\n", ret);

        virtio_device_ready(to_virtio_dev_t(&pci_vn));
        pci_d("virtio_device_ready:%d\n", ret);

//        virtio_irq_enable(to_virtio_dev_t(&pci_vn));
        bool virtio_had_irq(struct virtio_device *dev);

        pci_d("had_irq:%d\n", virtio_had_irq(to_virtio_dev_t(&pci_vn)));
        set_timeout(1000, vn_timeout);
        sti();

    }
    return OK;
}
