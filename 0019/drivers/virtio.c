#include "virtio.h"
#include "pci_device.h"
void virtio_setup_features(virtio_drv_t p);
void virtio_conf_read(virtio_drv_t pd, u32 offset, void *buf, int length)
{
    unsigned char *ptr = (unsigned char *)(buf);

    for (int i = 0; i < length; i++)
        ptr[i] = pci_bar_readb(pd->_bar1, offset + i);
}

void virtio_conf_write(virtio_drv_t pd, u32 offset, void *buf, int length)
{
    u8 *ptr = (u8 *) (buf);

    for (int i = 0; i < length; i++)
        pci_bar_writeb(pd->_bar1, offset + i, ptr[i]);
}

void virtio_driver_init(virtio_drv_t pd)
{
    if(!virtio_parse_pci_config(pd)){
        panic("can't virtio parse pci config");
    }
    set_pci_bus_master(to_pci_device_t(pd), true);
    pci_msix_enable(to_pci_device_t(pd));
    virtio_reset_host_side(pd);
    virtio_setup_features(pd);
}

bool virtio_parse_pci_config(virtio_drv_t p)
{
    pci_device_parse_config((to_pci_device_t(p)));
    p->_bar1 = pci_get_bar(&p->dev, 1);
    if (!p->_bar1) {
pci_d("p->_bar1:%d zero\n",p->_bar1);
        return false;
    }                           // Check ABI version
    u8 rev = pci_get_revision_id(to_pci_device_t(p));

    if (rev != VIRTIO_PCI_ABI_VERSION) {
        panic("Wrong virtio revision=%x", rev);
        return false;
    }

    // Check device ID
    u16 dev_id = pci_get_device_id(to_pci_device_t(p));

    if ((dev_id < VIRTIO_PCI_ID_MIN) || (dev_id > VIRTIO_PCI_ID_MAX)) {
        panic("Wrong virtio dev id %x", dev_id);
        return false;
    }

    return true;

}
static u32 virtio_get_device_features(virtio_drv_t p)
{
    return virtio_conf_readl(p,VIRTIO_PCI_HOST_FEATURES);
}

static void virtio_set_guest_features(virtio_drv_t p,u32 features)
{
    virtio_conf_writel(p,VIRTIO_PCI_GUEST_FEATURES, features);
}
void virtio_setup_features(virtio_drv_t p)
{
    u32 dev_features = virtio_get_device_features(p);
    u32 drv_features = virtio_get_driver_features();

    u32 subset = dev_features & drv_features;

    //notify the host about the features in used according
    //to the virtio spec
    for (int i = 0; i < 32; i++)
        if (subset & (1 << i))
            pci_d("%s: found feature intersec of bit %d\n", __FUNCTION__,  i);

    if (subset & (1 << VIRTIO_RING_F_INDIRECT_DESC))
       p->_cap_indirect_buf=true; 

    if (subset & (1 << VIRTIO_RING_F_EVENT_IDX))
            p->_cap_event_idx=true;

    virtio_set_guest_features(p,subset);
}
