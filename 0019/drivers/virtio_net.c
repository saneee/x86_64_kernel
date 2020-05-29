#include "pci_device.h"
#include "virtio.h"
#include "virtio_net.h"
#include <asm/apic.h>
static struct virtio_net pci_vn;
bool msix_unmask_entry(pci_device_t pci,int);
void msix_unmask_all(pci_device_t pci);
void virtio_net_read_config(struct virtio_net * p)
{
    u32 offset=virtio_pci_config_offset(to_virtio_drv_t(p));
    virtio_conf_read(to_virtio_drv_t(p),offset,&p->_config,sizeof(p->_config));
    if(virtio_get_guest_feature_bit(to_virtio_drv_t(p),VIRTIO_NET_F_MAC)){
    printk("The mac addr of the device is: %x:%x:%x:%x:%x:%x\n",
(u32)p->_config.mac[0],
                (u32)p->_config.mac[1],
                (u32)p->_config.mac[2],
                (u32)p->_config.mac[3],
                (u32)p->_config.mac[4],
                (u32)p->_config.mac[5]);

    }
 printk("The mac addr of the device offset:%lx is: %x:%x:%x:%x:%x:%x\n",
offset,(u32)p->_config.mac[0],
                (u32)p->_config.mac[1],
                (u32)p->_config.mac[2],
                (u32)p->_config.mac[3],
                (u32)p->_config.mac[4],
                (u32)p->_config.mac[5]);

}
void init_virtio_net()
{
    bool found = false;

    for (int b = 0; b < 255; b++) {
        for (int s = 0; s < 31; s++) {
            for (int f = 0; f < 7; f++) {
                uint16_t vid = read_pci_config_word(b, s, f, 0);
                uint16_t did = read_pci_config_word(b, s, f, 2);

                if (vid != 0xffff) {
                    printf("b:%d,s:%d,f:%d,vid:%lx,%lx\n", b, s, f, vid, did);
                    if (vid == 0x1af4 && did == 0x1000) {
                        pci_set_bdf(to_pci_device_t(&pci_vn), b, s, f);
                        found = true;
                    }
                }
            }
        }
    }
    if (found) {
//        found = virtio_parse_pci_config(to_virtio_drv_t(&pci_vn));
virtio_driver_init(to_virtio_drv_t(&pci_vn));
pci_dump_config(to_pci_device_t(&pci_vn));

virtio_net_read_config(&pci_vn);
 printk("found:%d,%d,%d\n", found, pci_vn.dev.dev._have_msix,
               pci_vn.dev.dev._msix_enabled);
pci_dump_config(to_pci_device_t(&pci_vn));
int irq=pci_get_interrupt_line(to_pci_device_t(&pci_vn));
ioapic_enable(irq,0);
msix_unmask_entry(to_pci_device_t(&pci_vn),irq);
msix_unmask_all(to_pci_device_t(&pci_vn));

sti();

    }

}
