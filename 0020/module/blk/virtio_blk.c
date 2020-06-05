#include <yaos/types.h>
#include <drivers/virtio.h>
#include <yaoscall/malloc.h>
#include <yaoscall/page.h>
#include "virtio_blk.h"
#include <asm/apic.h>
#include <errno.h>
#include <yaos/queue.h>
#include <yaos/assert.h>
#include <string.h>
#include <yaos/time.h>
#include <yaos/kernel.h>
#include <asm/irq.h>
#include <yaos/msi.h>
#include <yaos/smp.h>
#include <yaos/sched.h>
#include <drivers/pci_device.h>
#include <yaos/module.h>
static const char *const name = "virtio-blk";
static struct virtio_blk pci_vblk;
static void virtio_blk_check_queue(void);

static void irq_handler(int n)
{
    uchar isr = 
    virtio_conf_readb(to_virtio_dev_t(&pci_vblk), VIRTIO_PCI_ISR);
    ack_lapic_irq();
    virtio_queue_disable_intr(virtio_get_queue(to_virtio_dev_t(&pci_vblk), 0));
    printk("****virtio_blk_irq_handler**** irq:%d,isr:%d cpu:%d\n", n,isr,smp_processor_id());
    virtio_blk_check_queue();
}
void virtio_blk_read_config(struct virtio_blk *p)
{
    virtio_dev_t pvirtio = to_virtio_dev_t(p);
    u32 offset = virtio_pci_config_offset(to_virtio_dev_t(p));

    virtio_conf_read(pvirtio, offset, &p->_config,
                     sizeof(p->_config));
    if (virtio_get_guest_feature_bit(to_virtio_dev_t(p), VIRTIO_BLK_F_SIZE_MAX)) {
        pci_d("VIRTIO_BLK_F_SIZE_MAX:%x\n",p->_config.size_max);

    }
    if (virtio_get_guest_feature_bit(to_virtio_dev_t(p), VIRTIO_BLK_F_SEG_MAX)) {
        pci_d("VIRTIO_BLK_F_SEG_MAX:%x\n",p->_config.seg_max);

    }
    if (virtio_get_guest_feature_bit(to_virtio_dev_t(p), VIRTIO_BLK_F_GEOMETRY)) {
        pci_d("VIRTIO_BLK cylinders:%x,heads:%x,sectors:%x\n",
             p->_config.geometry.cylinders,
             p->_config.geometry.heads,p->_config.geometry.sectors);

    }
    if (virtio_get_guest_feature_bit(to_virtio_dev_t(p), VIRTIO_BLK_F_BLK_SIZE)) {
        pci_d("VIRTIO_BLK_F_BLK_SIZE:%x\n",p->_config.blk_size);

    }
    if (virtio_get_guest_feature_bit(to_virtio_dev_t(p), VIRTIO_BLK_F_TOPOLOGY)) {
        pci_d("VIRTIO_BLK topology,physical_block_exp:%x,alignment:%x,min_io_size:%x,opt_io_size:%x\n",
             p->_config.physical_block_exp,p->_config.alignment_offset,
             p->_config.min_io_size,p->_config.opt_io_size);

    }
    if (virtio_get_guest_feature_bit(to_virtio_dev_t(p), VIRTIO_BLK_F_CONFIG_WCE)) {
        pci_d("VIRTIO_BLK_F_WCE:%x\n",p->_config.wce);

    }
    if (virtio_get_guest_feature_bit(to_virtio_dev_t(p), VIRTIO_BLK_F_RO)) {
        pci_d("VIRTIO_BLK readonly\n");

    }


}
static char buf[512];
//static uchar status = 0;
struct blk_req {
    struct blk_outhdr hdr;
    pthread thread;
    uchar status;
};
static struct blk_req rq;
static void virtio_blk_read(ulong sector)
{
    struct addr_size phys[3];
    rq.hdr.type = VIRTIO_BLK_T_IN;
    rq.hdr.ioprio = 0;
    rq.hdr.sector = sector;
    rq.thread = current;
    phys[0].vp_addr = V2P((ulong)&rq.hdr);
    phys[0].vp_size = sizeof(rq.hdr);
    phys[0].vp_flags = VRING_DESC_F_READ;
    phys[1].vp_addr = V2P((ulong)buf);
    phys[1].vp_size = 512;
    phys[1].vp_flags = VRING_DESC_F_WRITE;
    uchar status = 0;
    phys[2].vp_addr = V2P((ulong)&rq.status);
    phys[2].vp_size = sizeof(status);    
    phys[2].vp_flags = VRING_DESC_F_WRITE;
    virtio_to_queue(to_virtio_dev_t(&pci_vblk), 0, phys, 3, &rq);
    suspend_thread();

}
static void virtio_blk_check_queue(void)
{
    struct blk_req *p;
    size_t len;
void virtio_get_buf_finalize(virtio_dev_t dev, int qidx);
    /* Put the received packets into the recv list */
    while (virtio_from_queue(to_virtio_dev_t(&pci_vblk),0 , (void **)&p, &len)
           == 0) {
        pci_d("virtio_blk_from_queue:%lx,len:%x,status:%d,thread:%s\n", p, len,p->status,p->thread->name);
        wake_up_thread(p->thread); 
    }

}
__used static void vblk_timeout(u64 nowmsec,void *param)
{
    set_timeout_nsec(5000000000, vblk_timeout,param);
    pci_d("vblktimeout:now sec:%d\n", nowmsec/1000000000);
    //dump_mem(buf,512);
    msi_register_one(to_pci_device_t(&pci_vblk),0,irq_handler);

    virtio_blk_read(2);

    virtio_blk_check_queue();
}

int init_virtio_blk()
{
    virtio_dev_t dev = to_virtio_dev_t(&pci_vblk);

    if (virtio_setup_device(dev, VIRTIO_BLK_DEVICE_ID, 1)) {
        virtio_blk_read_config(&pci_vblk);
        //virtio_driver_init(dev);


        pci_dump_config(to_pci_device_t(&pci_vblk));
        msi_register_one(to_pci_device_t(&pci_vblk),0,irq_handler);


        virtio_device_ready(to_virtio_dev_t(&pci_vblk));
        memset(buf,0x12,512);
        //set_timeout_nsec(1000000000UL, vblk_timeout,dev);
        virtio_blk_read(1);
        dump_mem(buf,512);
        virtio_blk_read(1);
        dump_mem(buf,512);

    }
    return OK;
}
DECLARE_MODULE(virtio_blk_drivers, 0, main);


__used static int main(module_t m, ulong t, void *arg)
{
    modeventtype_t env = (modeventtype_t) t;
    printk("virtiomain env:%d,%d\n",env,MOD_BPDEVICE);
    if (env == MOD_BPLOAD) {
        printk("virtio_blk_drivers, %lx,%lx,%lx\n", m, t, arg);
    } else if(env == MOD_BPDEVICE) {
        printk("init_virtio_net\n");
        init_virtio_blk();
    }
    return MOD_ERR_OK;          //MOD_ERR_NEXTLOOP;
}
