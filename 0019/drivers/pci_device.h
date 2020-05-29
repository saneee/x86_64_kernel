#ifndef _DRIVERS_PCI_DEVICE_H
#define _DRIVERS_PCI_DEVICE_H
#include <yaos/types.h>
#include <yaos/device.h>
#include <asm/pci.h>
#include "pci_function.h"
#include <yaos/printk.h>
#define MAX_PCI_BARS 6
#define pci_d(...) printk(__VA_ARGS__)
struct pci_device {
    struct device dev;
    u8 _bus;
    u8 _device;
    u8 _func;
    u8 _revision_id;
    u8 _header_type;
    u8 _base_class_code;
    u8 _sub_class_code;
    u8 _programming_interface;
    u16 _device_id;
    u16 _vendor_id;
    bool _have_msix;
    struct pcicfg_msix _msix;
    bool _msix_enabled;
    bool _have_msi;
    struct pcicfg_msi _msi;
    bool _msi_enabled;
    struct pci_bar _bars[MAX_PCI_BARS];
    u16 _subsystem_vid;
    u16 _subsystem_id;

};
typedef struct pci_device *pci_device_t;
static  inline u8 pci_get_revision_id(pci_device_t pci)
{
   return pci->_revision_id;
}
static  inline u16 pci_get_device_id(pci_device_t pci)
{
   return pci->_device_id;
}

static inline bool pci_is_msix_enabled(pci_device_t pci)
{
    return pci->_msix_enabled;
}
static inline bool pci_is_msix(pci_device_t pci)
{
    return pci->_have_msix;
}
static inline bool pci_is_msi_enabled(pci_device_t pci)
{
    return pci->_msi_enabled;
}
static inline bool pci_is_msi(pci_device_t pci)
{
    return pci->_have_msi;
}

static inline device_t pci_to_device(pci_device_t pci)
{
    return &pci->dev;
}

static inline u8 pci_readb(pci_device_t pci, u8 offset)
{
    return read_pci_config_byte(pci->_bus, pci->_device, pci->_func, offset);
}

static inline u16 pci_readw(pci_device_t pci, u8 offset)
{
    return read_pci_config_word(pci->_bus, pci->_device, pci->_func, offset);
}

static inline u32 pci_readl(pci_device_t pci, u8 offset)
{
    return read_pci_config(pci->_bus, pci->_device, pci->_func, offset);
}

static inline void pci_writeb(pci_device_t pci, u8 offset, u8 val)
{
    write_pci_config_byte(pci->_bus, pci->_device, pci->_func, offset, val);
}

static inline void pci_writew(pci_device_t pci, u8 offset, u16 val)
{
    write_pci_config_word(pci->_bus, pci->_device, pci->_func, offset, val);
}

static inline void pci_writel(pci_device_t pci, u8 offset, u32 val)
{
    write_pci_config(pci->_bus, pci->_device, pci->_func, offset, val);
}

static inline void pci_set_bdf(pci_device_t pci, u8 bus, u8 device, u8 func)
{
    pci->_bus = bus;
    pci->_device = device;
    pci->_func = func;
}

static inline u16 pci_get_command(pci_device_t pci)
{
    return pci_readw(pci, PCI_CFG_COMMAND);
}

static inline void pci_set_command(pci_device_t pci, u16 command)
{
    pci_writew(pci, PCI_CFG_COMMAND, command);
}

static inline u16 pci_get_status(pci_device_t pci)
{

    return pci_readw(pci, PCI_CFG_STATUS);
}

static inline void pci_set_status(pci_device_t pci, u16 status)
{
    return pci_writew(pci, PCI_CFG_COMMAND, status);
}

static inline bool pci_get_bus_master(pci_device_t pci)
{
    u16 command = pci_get_command(pci);

    return command & PCI_COMMAND_BUS_MASTER;
}
static inline void pci_disable_intx(pci_device_t pci)
{
    u16 command = pci_get_command(pci);
    command |= PCI_COMMAND_INTX_DISABLE;
    pci_set_command(pci,command);


}

static inline pci_device_t to_pci_device_t(void *p)
{
    return (pci_device_t) p;
}
static inline void msix_set_control(pci_device_t pci,u16 ctrl)
{
    pci_writew(pci,pci->_msix.msix_location+PCIR_MSIX_CTRL,ctrl);
}
static inline u16 msix_get_control(pci_device_t pci)
{
    return pci_readw(pci,pci->_msix.msix_location+PCIR_MSIX_CTRL);
}
static inline u8 pci_get_interrupt_line(pci_device_t pci)
{
    return pci_readb(pci,PCI_CFG_INTERRUPT_LINE);
}
static inline void msi_set_control(pci_device_t pci,u16 ctrl)
{
    pci_writew(pci,pci->_msi.msi_location+PCIR_MSI_CTRL,ctrl);
}
static inline u16 msi_get_control(pci_device_t pci)
{
    return pci_readw(pci,pci->_msi.msi_location+PCIR_MSI_CTRL);
}

extern bool parse_pci_config(pci_device_t pci);
extern bool pci_device_parse_config(pci_device_t pci);
extern void set_pci_bars_enable(pci_device_t pci, bool mem, bool io);
extern struct pci_bar *pci_get_bar(pci_device_t pci, int n);
extern void set_pci_bus_master(pci_device_t pci, bool master);
extern void pci_msix_enable(pci_device_t pci);
extern void pci_dump_config(pci_device_t pci);
extern bool find_pci_by_baseclass(pci_device_t pci);
extern bool find_pci_by_subclass(pci_device_t pci);

#endif
