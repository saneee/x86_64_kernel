#include <drivers/pci_device.h>
#include <yaos/compiler.h>
#include <yaos/assert.h>
#include <asm/irq.h>
#include <yaos/msi.h>
void set_pci_bus_master(pci_device_t pci, bool master)
{
    u16 command = pci_get_command(pci);

    command =
        (master) ?
        command | PCI_COMMAND_BUS_MASTER : command & ~PCI_COMMAND_BUS_MASTER;
    pci_set_command(pci, command);
}

void set_pci_bars_enable(pci_device_t pci, bool mem, bool io)
{
    u16 command = pci_get_command(pci);

    if (mem) {
        command |= PCI_COMMAND_BAR_MEM_ENABLE;
    }
    if (io) {
        command |= PCI_COMMAND_BAR_IO_ENABLE;
    }
    pci_set_command(pci, command);

}
void disable_intx(pci_device_t pci)
{
    u16 command = pci_get_command(pci);
    command |= PCI_COMMAND_INTX_DISABLE;
    pci_set_command(pci, command);
}
bool parse_pci_msix(pci_device_t pci, u8 off)
{
// Used for parsing MSI-x
    u32 val = 0;

    // Location within the configuration space
    pci->_msix.msix_location = off;
    pci->_msix.msix_ctrl = pci_readw(pci, off + PCIR_MSIX_CTRL);
    pci->_msix.msix_msgnum =
        (pci->_msix.msix_ctrl & PCIM_MSIXCTRL_TABLE_SIZE) + 1;
    val = pci_readl(pci, off + PCIR_MSIX_TABLE);
    pci->_msix.msix_table_bar = val & PCIM_MSIX_BIR_MASK;
    printk("msix_table_bar:%d,%d\n", pci->_msix.msix_table_bar, val);
    pci->_msix.msix_table_offset = val & ~PCIM_MSIX_BIR_MASK;
    val = pci_readl(pci, off + PCIR_MSIX_PBA);
    pci->_msix.msix_pba_bar = val & PCIM_MSIX_BIR_MASK;
    pci->_msix.msix_pba_offset = val & ~PCIM_MSIX_BIR_MASK;

    // We've found an MSI-x capability
    pci->_have_msix = true;

    return true;

}

bool parse_pci_msi(pci_device_t pci, u8 off)
{
    // Location within the configuration space
    pci->_msi.msi_location = off;
    pci->_msi.msi_ctrl = pci_readw(pci, off + PCIR_MSI_CTRL);
    // TODO: support multiple MSI message
    pci->_msi.msi_msgnum = 1;

    if (pci->_msi.msi_ctrl & (1 << 7)) {
        pci->_msi.is_64_address = true;
    }
    else {
        pci->_msi.is_64_address = false;
    }

    if (pci->_msi.msi_ctrl & (1 << 8)) {
        pci->_msi.is_vector_mask = true;
    }
    else {
        pci->_msi.is_vector_mask = false;
    }

    // We've found an MSI capability
    pci->_have_msi = true;

    return true;

}

u8 find_capability(pci_device_t pci, u8 cap_id)
{
    u8 capabilities_base = pci_readb(pci, PCI_CAPABILITIES_PTR);
    u8 off = capabilities_base;
    u8 bad_offset = 0xFF;
    u8 max_capabilities = 0xF0;
    u8 ctr = 0;

    while (off != 0) {
        // Read capability
        u8 capability = pci_readb(pci, off + PCI_CAP_OFF_ID);

        if (capability == cap_id) {
            return off;
        }

        ctr++;
        if (ctr > max_capabilities) {
            return bad_offset;
        }

        // Next
        off = pci_readb(pci, off + PCI_CAP_OFF_NEXT);
    }

    return bad_offset;
}

bool parse_pci_capabilities(pci_device_t pci)
{
    // Parse MSI-X
    u8 off = find_capability(pci, PCI_CAP_MSIX);

    if (off != 0xFF) {
        bool msi_ok = parse_pci_msix(pci, off);

        return msi_ok;
    }

    // Parse MSI
    off = find_capability(pci, PCI_CAP_MSI);
    if (off != 0xFF) {
        return parse_pci_msi(pci, off);
    }

    return true;

}

bool parse_pci_config(pci_device_t pci)
{
    pci->_device_id = pci_readw(pci, PCI_CFG_DEVICE_ID);
    pci->_vendor_id = pci_readw(pci, PCI_CFG_VENDOR_ID);
    pci->_revision_id = pci_readb(pci, PCI_CFG_REVISION_ID);
    pci->_header_type = pci_readb(pci, PCI_CFG_HEADER_TYPE);
    pci->_base_class_code = pci_readb(pci, PCI_CFG_CLASS_CODE0);
    pci->_sub_class_code = pci_readb(pci, PCI_CFG_CLASS_CODE1);
    pci->_programming_interface = pci_readb(pci, PCI_CFG_CLASS_CODE2);
    return parse_pci_capabilities(pci);
}

bool pci_device_parse_config(pci_device_t pci)
{
    u32 pos = PCI_CFG_BAR_1;
    int idx = 1;

    for (int i = 0; i < 6; i++) {
        pci->_bars[i]._dev = (pci_device_t) 0;
    }
    parse_pci_config(pci);
    pci->_subsystem_vid = pci_readw(pci, PCI_CFG_SUBSYSTEM_VENDOR_ID);
    pci->_subsystem_id = pci_readw(pci, PCI_CFG_SUBSYSTEM_ID);
    set_pci_bars_enable(pci, true, true);
    while (pos <= PCI_CFG_BAR_6) {
        u32 bar_v = pci_readl(pci, pos);

//pci_d("bar_v:%x\n",bar_v);
        struct pci_bar *pbar;

        if (bar_v == 0) {
            pos += 4;
            idx++;
            continue;
        }
        ASSERT(idx <= 7);
        printk("bar:%d,pos:%d\n", idx, pos);
        pbar = &pci->_bars[idx - 1];
        pci_bar_init(pbar, pci, pos);
        idx++;

        pos += pci_bar_is_64(pbar) ? idx++, 8 : 4;
    }

    return true;

}

struct pci_bar *pci_get_bar(pci_device_t pci, int n)
{
    ASSERT(n <= 6);
    if (!pci->_bars[n - 1]._dev)
        return (struct pci_bar *)0;
    return &pci->_bars[n - 1];
}

u64 msix_get_table(pci_device_t pci)
{
    struct pci_bar *msix_bar = pci_get_bar(pci, pci->_msix.msix_table_bar + 1);

    if (!msix_bar)
        return 0;
pci_dump_config(pci);
    printk("pci:%016lx,pci_bar_get_mmio:%016lx,offset:%lx\n",pci,pci_bar_get_mmio(msix_bar),pci->_msix.msix_table_offset);
    return (u64) (pci_bar_get_mmio(msix_bar) + pci->_msix.msix_table_offset);
}


bool msi_mask_entry(pci_device_t pci, int entry_id)
{
    if (!pci_is_msi(pci)) {
        return false;
    }
    if (entry_id >= pci->_msi.msi_msgnum) {
        return false;
    }
    if (pci->_msi.is_vector_mask) {
        // 64 bits address enabled?
        if (pci->_msi.is_64_address) {
            u8 reg = pci->_msi.msi_location + PCIR_MSI_MASK_64;
            u32 mask = pci_readl(pci,reg);
            mask |= 1 << entry_id;
            pci_writel(pci, reg, mask);
        } else {
            u8 reg = pci->_msi.msi_location + PCIR_MSI_MASK_32;
            u32 mask = pci_readl(pci, reg);
            mask |= 1 << entry_id;
            pci_writel(pci, reg, mask);
        }
    }

    return true;

}
bool msi_unmask_entry(pci_device_t pci, int entry_id)
{
    if (!pci_is_msi(pci)) {
        return false;
    }
    if (entry_id >= pci->_msi.msi_msgnum) {
        return false;
    }
    if (pci->_msi.is_vector_mask) {
       // 64 bits address enabled?
        if (pci->_msi.is_64_address) {
            u8 reg = pci->_msi.msi_location + PCIR_MSI_MASK_64;
            u32 mask = pci_readl(pci, reg);
            mask &= ~(1 << entry_id);
            pci_writel(pci, reg, mask);
        } else {
            u8 reg = pci->_msi.msi_location + PCIR_MSI_MASK_32;
            u32 mask = pci_readl(pci, reg);
            mask &= ~(1 << entry_id);
            pci_writel(pci, reg, mask);
        }
    }
    return true;
}
bool msi_write_entry(pci_device_t pci, int entry_id, struct msi_msg *msg)
{
    if (!pci_is_msi(pci)) {
        return false;
    }
    if (entry_id >= pci->_msi.msi_msgnum) {
        return false;
    }
    if (pci->_msi.is_64_address) {
        pci_writel(pci, pci->_msi.msi_location + PCIR_MSI_ADDR, msg->address_lo );
        pci_writel(pci, pci->_msi.msi_location + PCIR_MSI_UADDR, msg->address_hi );
        pci_writel(pci, pci->_msi.msi_location + PCIR_MSI_DATA_64, msg->data);

    } else {
        pci_writel(pci, pci->_msi.msi_location + PCIR_MSI_ADDR, msg->address_lo );
        pci_writel(pci, pci->_msi.msi_location + PCIR_MSI_DATA_64, msg->data);

    }

    return true;

}
void msi_enable(pci_device_t pci)
{
    if (!pci_is_msi(pci)) {
        return;
    }

        // Disabled intx assertions which is turned on by default
    disable_intx(pci);

    u16 ctrl = msi_get_control(pci);
    ctrl |= PCIR_MSI_CTRL_ME;

    // Mask all individual entries
    for (int i = 0; i< pci->_msi.msi_msgnum; i++) {
        msi_mask_entry(pci, i);
    }

    msi_set_control(pci, ctrl);

   pci->_msi_enabled = true;
}    
void msi_disable(pci_device_t pci)
{
    if (!pci_is_msi(pci)) {
        return;
    }


    u16 ctrl = msi_get_control(pci);
    ctrl &= ~PCIR_MSI_CTRL_ME;

    msi_set_control(pci,ctrl);

   pci->_msi_enabled = false;
}
bool msix_mask_entry(pci_device_t pci, int entry_id)
{
    if (!pci_is_msix(pci)) {
        return false;
    }

    if (entry_id >= pci->_msix.msix_msgnum) {
        return false;
    }

    u64 entryaddr = msix_get_table(pci) + (entry_id * MSIX_ENTRY_SIZE);
    u64 ctrl = entryaddr + (u8) MSIX_ENTRY_CONTROL;

    u32 ctrl_data = mmio_getl(ctrl);

    ctrl_data |= (1 << MSIX_ENTRY_CONTROL_MASK_BIT);
    mmio_setl(ctrl, ctrl_data);

    return true;
}

bool msix_unmask_entry(pci_device_t pci, int entry_id)
{
    if (!pci_is_msix(pci)) {
        return false;
    }

    if (entry_id >= pci->_msix.msix_msgnum) {
        return false;
    }

    u64 entryaddr = msix_get_table(pci) + (entry_id * MSIX_ENTRY_SIZE);
    u64 ctrl = entryaddr + (u8) MSIX_ENTRY_CONTROL;

    u32 ctrl_data = mmio_getl(ctrl);

    ctrl_data &= ~(1 << MSIX_ENTRY_CONTROL_MASK_BIT);
    mmio_setl(ctrl, ctrl_data);

    return true;

}

void msix_unmask_all(pci_device_t pci)
{
    if (!pci_is_msix(pci)) {
        return;
    }

    u16 ctrl = msix_get_control(pci);

    ctrl &= ~PCIM_MSIXCTRL_FUNCTION_MASK;
    msix_set_control(pci, ctrl);
}

void pci_msix_enable(pci_device_t pci)
{
    if (!pci_is_msix(pci))
        return;
    struct pci_bar *msix_bar = pci_get_bar(pci, pci->_msix.msix_table_bar + 1);

    printk("msix_bar:%lx,%d\n", msix_bar, pci->_msix.msix_table_bar);
    if (!msix_bar)
        return;
    pci_bar_map(msix_bar);
    pci_disable_intx(pci);

    // Only after enabling msix, the access to the pci bar is permitted
    // so we enable it while masking all interrupts in the msix ctrl reg
    u16 ctrl = msix_get_control(pci);
printk("ctrl:%x,",ctrl);
    ctrl |= PCIM_MSIXCTRL_MSIX_ENABLE;
    ctrl |= PCIM_MSIXCTRL_FUNCTION_MASK;
printk(" %x",ctrl);
    msix_set_control(pci, ctrl);
 ctrl = msix_get_control(pci);
printk(" %x,num:%d\n",ctrl,pci->_msix.msix_msgnum);

    // Mask all individual entries
    for (int i = 0; i < pci->_msix.msix_msgnum; i++) {
        msix_mask_entry(pci, i);
    }

    // After all individual entries are masked,
    // Unmask the main block
    ctrl &= ~PCIM_MSIXCTRL_FUNCTION_MASK;
    msix_set_control(pci, ctrl);

    pci->_msix_enabled = true;

}

bool pci_msix_write_entry(pci_device_t pci, int entry, u64 addr, u32 data)
{
    if (unlikely(!pci_is_msix(pci))) {
        pci_d("pci_msix_write_entry while is not msix\n");
        return false;
    }
    if (unlikely((entry >= pci->_msix.msix_msgnum))) {
        pci_d("entry too big:%d,msgnum:%d\n", entry, pci->_msix.msix_msgnum);
        return false;
    }
    u64 entryaddr = msix_get_table(pci) + (entry * MSIX_ENTRY_SIZE);
printk("msix_write_entry addr:%lx\n",entryaddr);
    mmio_setq(entryaddr + (u8) MSIX_ENTRY_ADDR, addr);
    mmio_setl(entryaddr + (u8) MSIX_ENTRY_DATA, data);
    return true;
}

void pci_dump_config(pci_device_t pci)
{
    pci_d("[%x:%x.%x] vid:id = %x:%x\n",
          (u16) pci->_bus, (u16) pci->_device, (u16) pci->_func,
          pci->_vendor_id, pci->_device_id);

    // PCI BARs
    for (int bar_idx = 1; bar_idx <= 6; bar_idx++) {
        struct pci_bar *bar = pci_get_bar(pci, bar_idx);

        if (bar) {
            pci_d("    bar[%d]: %sbits addr=%p size=%x\n",
                  bar_idx, (pci_bar_is_64(bar) ? "64" : "32"),
                  pci_bar_get_addr64(bar), pci_bar_get_size(bar));
        }
    }
    pci_d("    IRQ = %d\n", (u16) pci_get_interrupt_line(pci));

    // MSI-x
    if (pci->_have_msix) {
        pci_d("    Have MSI-X!");
        pci_d("        msix_location: %d", (u16) pci->_msix.msix_location);
        pci_d("        msix_ctrl: %d", pci->_msix.msix_ctrl);
        pci_d("        msix_msgnum: %d", pci->_msix.msix_msgnum);
        pci_d("        msix_table_bar: %d", (u16) pci->_msix.msix_table_bar);
        pci_d("        msix_table_offset: %d", pci->_msix.msix_table_offset);
        pci_d("        msix_pba_bar: %d", (u16) pci->_msix.msix_pba_bar);
        pci_d("        msix_pba_offset: %d\n", pci->_msix.msix_pba_offset);
    }

}
