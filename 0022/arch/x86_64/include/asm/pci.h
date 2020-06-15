#ifndef _ASM_PCI_H
#define _ASM_PCI_H
#include <pci_def.h>
#include <asm/cpu.h>
#include <yaos/printk.h>
#include <yaos/types.h>
#include <yaos/queue.h>
struct pci_device;
typedef struct pci_device *pci_device_t;
static inline u32 build_config_address(u8 bus, u8 slot, u8 func, u8 offset)
{
    return bus << PCI_BUS_OFFSET | slot << PCI_SLOT_OFFSET |
        func << PCI_FUNC_OFFSET | (offset & ~0x03);
}

static inline void prepare_pci_config_access(u8 bus, u8 slot, u8 func,
	     u8 offset)
{
    u32 address = build_config_address(bus, slot, func, offset);

    outl(PCI_CONFIG_ADDRESS_ENABLE | address, PCI_CONFIG_ADDRESS);
}

static inline u32 read_pci_config(u8 bus, u8 slot, u8 func, u8 offset)
{
    prepare_pci_config_access(bus, slot, func, offset);
    return inl(PCI_CONFIG_DATA);
}

static inline u16 read_pci_config_word(u8 bus, u8 slot, u8 func, u8 offset)
{
    prepare_pci_config_access(bus, slot, func, offset);
    return inw(PCI_CONFIG_DATA + (offset & 0x02));
}

static inline u8 read_pci_config_byte(u8 bus, u8 slot, u8 func, u8 offset)
{
    prepare_pci_config_access(bus, slot, func, offset);
    return inb(PCI_CONFIG_DATA + (offset & 0x03));
}

static inline void write_pci_config(u8 bus, u8 slot, u8 func, u8 offset,
                                    u32 val)
{
    prepare_pci_config_access(bus, slot, func, offset);
    outl(val, PCI_CONFIG_DATA);
}

static inline void write_pci_config_word(u8 bus, u8 slot, u8 func, u8 offset,
	 u16 val)
{
    prepare_pci_config_access(bus, slot, func, offset);
    outw(val, PCI_CONFIG_DATA + (offset & 0x02));
}

static inline void write_pci_config_byte(u8 bus, u8 slot, u8 func, u8 offset,
	 u8 val)
{
    prepare_pci_config_access(bus, slot, func, offset);
    outb(val, PCI_CONFIG_DATA + (offset & 0x03));
}

static inline u32 arch_add_pci_bar(u32 val)
{
    return val;
}
#endif
