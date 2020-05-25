/*
 * Copyright (C) 2013 Cloudius Systems, Ltd.
 *
 * This work is open source software, licensed under the terms of the
 * BSD license as described in the LICENSE file in the top-level directory.
 */

#ifndef _DRIVERS_PCI_FUNCTION_H
#define _DRIVERS_PCI_FUNCTION_H

#include <yaos/types.h>
#include <asm/cpu.h>
#include <drivers/mmio.h>
#include <yaos/printk.h>
enum pci_bar_encoding_masks {
    // mmio, pio
    PCI_BAR_MEMORY_INDICATOR_MASK = (1 << 0),
    // 32bit, 64bit
    PCI_BAR_MEM_ADDR_SPACE_MASK = (1 << 1) | (1 << 2),
    PCI_BAR_PREFETCHABLE_MASK = (1 << 3),
    PCI_BAR_MEM_ADDR_LO_MASK = 0xFFFFFFF0,
    PCI_BAR_PIO_ADDR_MASK = 0xFFFFFFFC,
};

enum pci_bar_type_indicator {
    PCI_BAR_MMIO = 0x00,
    PCI_BAR_PIO = 0x01
};

enum pci_bar_prefetchable {
    PCI_BAR_NON_PREFETCHABLE = 0x00,
    PCI_BAR_PREFETCHABLE = 0x08
};

enum pci_bar_address_space {
    PCI_BAR_32BIT_ADDRESS = 0x00,
    PCI_BAR_32BIT_BELOW_1MB = 0x02,
    PCI_BAR_64BIT_ADDRESS = 0x04
};

struct pci_device;

        // read/write pci cfg registers to determine size
struct pci_bar {
    struct pci_device *_dev;

    // Offset to configuration space
    u8 _pos;

    // Base address
    u32 _addr_lo, _addr_hi;
    u64 _addr_64;
    u64 _addr_size;
    u64 _addr_mmio;

    bool _is_mmio;
    bool _is_64;
    bool _is_prefetchable;
};

static inline bool pci_bar_is_pio(struct pci_bar *pbar)
{
    return !pbar->_is_mmio;
}

static inline bool pci_bar_is_mmio(struct pci_bar *pbar)
{
    return pbar->_is_mmio;
}

static inline bool pci_bar_is_32(struct pci_bar *pbar)
{
    return !pbar->_is_64;
}

static inline bool pci_bar_is_64(struct pci_bar *pbar)
{
    return pbar->_is_64;
}

static inline bool pci_bar_is_prefetchable(struct pci_bar *pbar)
{
    return pbar->_is_prefetchable;
}

static inline u32 pci_bar_get_addr_lo(struct pci_bar *pbar)
{
    return pbar->_addr_lo;
}

static inline u32 pci_bar_get_addr_hi(struct pci_bar *pbar)
{
    return pbar->_addr_hi;
}

static inline u64 pci_bar_get_addr64(struct pci_bar *pbar)
{
    return pbar->_addr_64;
}

static inline u64 pci_bar_get_size(struct pci_bar *pbar)
{
    return pbar->_addr_size;
}

static inline void pci_bar_map(struct pci_bar *pbar)
{
    if (pbar->_is_mmio) {
        pbar->_addr_mmio =
            mmio_map(pci_bar_get_addr64(pbar), pci_bar_get_size(pbar));
    }
};

static inline void pci_bar_unmap(struct pci_bar *pbar)
{
};

static inline u64 pci_bar_get_mmio(struct pci_bar *pbar)
{
    return pbar->_addr_mmio;
}

        // Access the pio or mmio bar
static inline u64 pci_var_readq(struct pci_bar *pbar, u32 offset)
{
    if (pbar->_is_mmio) {
        return mmio_getq(pbar->_addr_mmio + offset);
    }
    else
        panic("64 bit read attempt from PIO area");
}

static inline u32 pci_bar_readl(struct pci_bar *pbar, u32 offset)
{
    if (pbar->_is_mmio) {
        return mmio_getl(pbar->_addr_mmio + offset);
    }
    else {
        return inl(pbar->_addr_lo + offset);
    }
}

static inline u16 pci_bar_readw(struct pci_bar *pbar, u32 offset)
{
    if (pbar->_is_mmio) {
        return mmio_getw(pbar->_addr_mmio + offset);
    }
    else {
        return inw(pbar->_addr_lo + offset);
    }
}

static inline u32 pci_bar_readb(struct pci_bar *pbar, u32 offset)
{
    if (pbar->_is_mmio) {
        return mmio_getb(pbar->_addr_mmio + offset);
    }
    else {
        return inb(pbar->_addr_lo + offset);
    }
}

static inline void pci_bar_writeq(struct pci_bar *pbar, u32 offset, u64 val)
{
    if (pbar->_is_mmio) {
        mmio_setq(pbar->_addr_mmio + offset, val);
    }
    else {
        panic("64 bit write attempt to PIO area");
    }

}

static inline void pci_bar_writel(struct pci_bar *pbar, u32 offset, u32 val)
{
    if (pbar->_is_mmio) {
        mmio_setl(pbar->_addr_mmio + offset, val);
    }
    else {
        outl(val, pbar->_addr_lo + offset);
    }

}

static inline void pci_bar_writew(struct pci_bar *pbar, u32 offset, u16 val)
{
    if (pbar->_is_mmio) {
        mmio_setw(pbar->_addr_mmio + offset, val);
    }
    else {
        outw(val, pbar->_addr_lo + offset);
    }

}

static inline void pci_bar_writeb(struct pci_bar *pbar, u32 offset, u8 val)
{
    if (pbar->_is_mmio) {
        mmio_setb(pbar->_addr_mmio + offset, val);
    }
    else {
        outb(val, pbar->_addr_lo + offset);
    }

}

    //  MSI-X definitions
enum msi_pci_conf {
    PCIR_MSI_CTRL = 0x2,
    PCIR_MSI_CTRL_ME = 1 << 0,
    PCIR_MSI_ADDR = 0x4,
    PCIR_MSI_UADDR = 0x8,
    PCIR_MSI_DATA_32 = 0x8,
    PCIR_MSI_DATA_64 = 0x0C,
    PCIR_MSI_MASK_32 = 0x0C,
    PCIR_MSI_MASK_64 = 0x10,
};

    //  MSI-X definitions
enum msix_pci_conf {
    PCIR_MSIX_CTRL = 0x2,
    PCIM_MSIXCTRL_MSIX_ENABLE = 0x8000,
    PCIM_MSIXCTRL_FUNCTION_MASK = 0x4000,
    PCIM_MSIXCTRL_TABLE_SIZE = 0x07FF,
    PCIR_MSIX_TABLE = 0x4,
    PCIR_MSIX_PBA = 0x8,
    PCIM_MSIX_BIR_MASK = 0x7,

    // Entry offsets
    MSIX_ENTRY_ADDR = 0,
    MSIX_ENTRY_ADDR_LO = 0,
    MSIX_ENTRY_ADDR_HI = 4,
    MSIX_ENTRY_DATA = 8,
    MSIX_ENTRY_CONTROL = 12,
    MSIX_ENTRY_SIZE = 16,
    MSIX_ENTRY_CONTROL_MASK_BIT = 0
};

struct pcicfg_msix {
    u16 msix_ctrl;              //  Message Control
    u16 msix_msgnum;            //  Number of messages
    u8 msix_location;           //  Offset of MSI-X capability registers.
    u8 msix_table_bar;          //  BAR containing vector table.
    u8 msix_pba_bar;            //  BAR containing PBA.
    u32 msix_table_offset;
    u32 msix_pba_offset;
};

struct pcicfg_msi {
    u8 msi_location;            //  Offset of MSI capability registers.
    u16 msi_ctrl;               //  Message Control
    u16 msi_msgnum;             //  Number of messages
    bool is_64_address;         //  64 bit address
    bool is_vector_mask;        //  Per-vector mask
};

enum pci_function_cfg_offsets {
    PCI_CFG_VENDOR_ID = 0x00,
    PCI_CFG_DEVICE_ID = 0x02,
    PCI_CFG_COMMAND = 0x04,
    PCI_CFG_STATUS = 0x06,
    PCI_CFG_REVISION_ID = 0x08,
    PCI_CFG_CLASS_CODE0 = 0x0B,
    PCI_CFG_CLASS_CODE1 = 0x0A,
    PCI_CFG_CLASS_CODE2 = 0x09,
    PCI_CFG_CACHELINE_SIZE = 0x0C,
    PCI_CFG_LATENCY_TIMER = 0x0D,
    PCI_CFG_HEADER_TYPE = 0x0E,
    PCI_CFG_BIST = 0x0F,
    PCI_CFG_BAR_1 = 0x10,
    PCI_CFG_BAR_2 = 0x14,
    PCI_CFG_BAR_3 = 0x18,
    PCI_CFG_BAR_4 = 0x1C,
    PCI_CFG_BAR_5 = 0x20,
    PCI_CFG_BAR_6 = 0x24,
    PCI_CFG_CARDBUS_CIS_PTR = 0x28,
    PCI_CFG_SUBSYSTEM_VENDOR_ID = 0x2C,
    PCI_CFG_SUBSYSTEM_ID = 0x2E,
    PCI_CFG_CAPABILITIES_PTR = 0x34,
    PCI_CFG_INTERRUPT_LINE = 0x3C,
    PCI_CFG_INTERRUPT_PIN = 0x3D
};

enum pci_command_bits {
    PCI_COMMAND_INTX_DISABLE = (1 << 10),
    PCI_COMMAND_BUS_MASTER = (1 << 2),
    PCI_COMMAND_BAR_MEM_ENABLE = (1 << 1),
    PCI_COMMAND_BAR_IO_ENABLE = (1 << 0)
};

enum pci_header_type {
    PCI_HDR_TYPE_DEVICE = 0x00,
    PCI_HDR_TYPE_BRIDGE = 0x01,
    PCI_HDR_TYPE_PCCARD = 0x02,
    PCI_HDR_TYPE_MASK = 0x03
};

        //  Capability Register Offsets
enum pci_capabilities_offsets {
    PCI_CAP_OFF_ID = 0x0,
    PCI_CAP_OFF_NEXT = 0x1
};

enum pci_capabilities {
    PCI_CAP_PM = 0x01,          // PCI Power Management
    PCI_CAP_AGP = 0x02,         // AGP
    PCI_CAP_VPD = 0x03,         // Vital Product Data
    PCI_CAP_SLOTID = 0x04,      // Slot Identification
    PCI_CAP_MSI = 0x05,         // Message Signaled Interrupts
    PCI_CAP_CHSWP = 0x06,       // CompactPCI Hot Swap
    PCI_CAP_PCIX = 0x07,        // PCI-X
    PCI_CAP_HT = 0x08,          // HyperTransport
    PCI_CAP_VENDOR = 0x09,      // Vendor Unique
    PCI_CAP_DEBUG = 0x0a,       // Debug port
    PCI_CAP_CRES = 0x0b,        // CompactPCI central resource control
    PCI_CAP_HOTPLUG = 0x0c,     // PCI Hot-Plug
    PCI_CAP_SUBVENDOR = 0x0d,   // PCI-PCI bridge subvendor ID
    PCI_CAP_AGP8X = 0x0e,       // AGP 8x
    PCI_CAP_SECDEV = 0x0f,      // Secure Device
    PCI_CAP_EXPRESS = 0x10,     // PCI Express
    PCI_CAP_MSIX = 0x11,        // MSI-X
    PCI_CAP_SATA = 0x12,        // SATA
    PCI_CAP_PCIAF = 0x13        // PCI Advanced Features
};

enum pci_class_codes {
    PCI_CLASS_STORAGE = 0x01,
    PCI_CLASS_DISPLAY = 0x03
};

enum pci_sub_class_codes {
    PCI_SUB_CLASS_STORAGE_SCSI = 0x00,
    PCI_SUB_CLASS_STORAGE_IDE = 0x01,
    PCI_SUB_CLASS_STORAGE_FLOPPY = 0x02,
    PCI_SUB_CLASS_STORAGE_IPI = 0x03,
    PCI_SUB_CLASS_STORAGE_RAID = 0x04,
    PCI_SUB_CLASS_STORAGE_ATA = 0x05,
    PCI_SUB_CLASS_STORAGE_AHCI = 0x06,
    PCI_SUB_CLASS_STORAGE_SAS = 0x07,
    PCI_SUB_CLASS_STORAGE_NVMC = 0x08,
};
extern u64 pci_bar_read_size(struct pci_bar *p);
extern void pci_bar_init(struct pci_bar *p,struct pci_device * pci,u32 pos);
#endif // PCI_DEVICE_H
