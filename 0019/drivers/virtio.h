/*
 * Copyright (C) 2013 Cloudius Systems, Ltd.
 *
 * This work is open source software, licensed under the terms of the
 * BSD license as described in the LICENSE file in the top-level directory.
 */

#ifndef _DRIVERS_VIRTIO_H
#define _DRIVERS_VIRTIO_H

#include "pci_function.h"
#include "pci_device.h"
enum VIRTIO_CONFIG {
    /* Status byte for guest to report progress, and synchronize features. */
    /* We have seen device and processed generic fields (VIRTIO_CONFIG_F_VIRTIO) */
    VIRTIO_CONFIG_S_ACKNOWLEDGE = 1,
    /* We have found a driver for the device. */
    VIRTIO_CONFIG_S_DRIVER = 2,
    /* Driver has used its parts of the config, and is happy */
    VIRTIO_CONFIG_S_DRIVER_OK = 4,
    /* We've given up on this device. */
    VIRTIO_CONFIG_S_FAILED = 0x80,
    /* Some virtio feature bits (currently bits 28 through 31) are reserved for the
     * transport being used (eg. virtio_ring), the rest are per-device feature
     * bits. */
    VIRTIO_TRANSPORT_F_START = 28,
    VIRTIO_TRANSPORT_F_END = 32,
    /* We support indirect buffer descriptors */
    VIRTIO_RING_F_INDIRECT_DESC = 28,
    /* The Guest publishes the used index for which it expects an interrupt
     * at the end of the avail ring. Host should ignore the avail->flags field. */
    /* The Host publishes the avail index for which it expects a kick
     * at the end of the used ring. Guest should ignore the used->flags field. */
    VIRTIO_RING_F_EVENT_IDX = 29,

    /* Do we get callbacks when the ring is completely used, even if we've
     * suppressed them? */
    VIRTIO_F_NOTIFY_ON_EMPTY = 24,
    /* A 32-bit r/o bitmask of the features supported by the host */
    VIRTIO_PCI_HOST_FEATURES = 0,
    /* A 32-bit r/w bitmask of features activated by the guest */
    VIRTIO_PCI_GUEST_FEATURES = 4,
    /* A 32-bit r/w PFN for the currently selected queue */
    VIRTIO_PCI_QUEUE_PFN = 8,
    /* A 16-bit r/o queue size for the currently selected queue */
    VIRTIO_PCI_QUEUE_NUM = 12,
    /* A 16-bit r/w queue selector */
    VIRTIO_PCI_QUEUE_SEL = 14,
    /* A 16-bit r/w queue notifier */
    VIRTIO_PCI_QUEUE_NOTIFY = 16,
    /* An 8-bit device status register.  */
    VIRTIO_PCI_STATUS = 18,
    /* An 8-bit r/o interrupt status register.  Reading the value will return the
     * current contents of the ISR and will also clear it.  This is effectively
     * a read-and-acknowledge. */
    VIRTIO_PCI_ISR = 19,
    /* The bit of the ISR which indicates a device configuration change. */
    VIRTIO_PCI_ISR_CONFIG = 0x2,
    /* MSI-X registers: only enabled if MSI-X is enabled. */
    /* A 16-bit vector for configuration changes. */
    VIRTIO_MSI_CONFIG_VECTOR = 20,
    /* A 16-bit vector for selected queue notifications. */
    VIRTIO_MSI_QUEUE_VECTOR = 22,
    /* Vector value used to disable MSI for queue */
    VIRTIO_MSI_NO_VECTOR = 0xffff,
    /* Virtio ABI version, this must match exactly */
    VIRTIO_PCI_ABI_VERSION = 0,
    /* How many bits to shift physical queue address written to QUEUE_PFN.
     * 12 is historical, and due to x86 page size. */
    VIRTIO_PCI_QUEUE_ADDR_SHIFT = 12,
    /* The alignment to use between consumer and producer parts of vring.
     * x86 pagesize again. */
    VIRTIO_PCI_VRING_ALIGN = 4096,

};

enum {
    VIRTIO_VENDOR_ID = 0x1af4,
    VIRTIO_PCI_ID_MIN = 0x1000,
    VIRTIO_PCI_ID_MAX = 0x103f,

    VIRTIO_ID_NET = 1,
    VIRTIO_ID_BLOCK = 2,
    VIRTIO_ID_CONSOLE = 3,
    VIRTIO_ID_RNG = 4,
    VIRTIO_ID_BALLOON = 5,
    VIRTIO_ID_RPMSG = 7,
    VIRTIO_ID_SCSI = 8,
    VIRTIO_ID_9P = 9,
    VIRTIO_ID_RPROC_SERIAL = 11,
};

#define VIRTIO_ALIGN(x) ((x + (VIRTIO_PCI_VRING_ALIGN-1)) & ~(VIRTIO_PCI_VRING_ALIGN-1))

static const unsigned max_virtqueues_nr = 64;
struct virtio_driver {
    struct pci_device dev;
    struct pci_bar *_bar1;
    bool _cap_indirect_buf;
    bool _cap_event_idx;
};
typedef struct virtio_driver *virtio_drv_t;
static inline u8 virtio_conf_readb(virtio_drv_t p, u32 offset)
{
    return pci_bar_readb(p->_bar1, offset);
};

static inline u16 virtio_conf_readw(virtio_drv_t p, u32 offset)
{
    return pci_bar_readw(p->_bar1, offset);
};

static inline u32 virtio_conf_readl(virtio_drv_t p, u32 offset)
{
    return pci_bar_readl(p->_bar1, offset);
};

static inline void virtio_conf_writeb(virtio_drv_t p, u32 offset, u8 val)
{
    pci_bar_writeb(p->_bar1, offset, val);
};

static inline void virtio_conf_writew(virtio_drv_t p, u32 offset, u16 val)
{
    pci_bar_writew(p->_bar1, offset, val);
};

static inline void virtio_conf_writel(virtio_drv_t p, u32 offset, u32 val)
{
    pci_bar_writel(p->_bar1, offset, val);
};

extern void virtio_conf_read(virtio_drv_t p, u32 offset, void *buf, int length);
extern void virtio_conf_write(virtio_drv_t p, u32 offset, void *buf,
                              int length);
extern bool virtio_parse_pci_config(virtio_drv_t p);
static inline void virtio_set_dev_status(virtio_drv_t p, u8 status)
{
    virtio_conf_writeb(p, VIRTIO_PCI_STATUS, status);
}

static inline void virtio_reset_host_side(virtio_drv_t p)
{
    virtio_set_dev_status(p, 0);
}

static inline virtio_drv_t to_virtio_drv_t(void *p)
{
    return (virtio_drv_t) p;
}

static inline bool virtio_get_config_bit(virtio_drv_t p, u32 offset, int bit)
{
    return virtio_conf_readl(p, (offset) & (1 << bit));
}

static inline bool virtio_get_guest_feature_bit(virtio_drv_t p, int bit)
{
    return virtio_get_config_bit(p, VIRTIO_PCI_GUEST_FEATURES, bit);
}

static inline u32 virtio_get_driver_features()
{
    return 1 << VIRTIO_RING_F_INDIRECT_DESC | 1 << VIRTIO_RING_F_EVENT_IDX;
}

static inline int virtio_pci_config_offset(virtio_drv_t p)
{
    return pci_is_msix_enabled(to_pci_device_t(p)) ? 24 : 20;
}

extern bool virtio_parse_pci_config(virtio_drv_t p);
extern void virtio_driver_init(virtio_drv_t pd);
#endif
