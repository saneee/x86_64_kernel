#include "../drivers/pci_device.h"

void kernel_start()
{
    struct pci_device pci = {._base_class_code = PCI_CLASS_DISPLAY};
    char *ptr;
    if (find_pci_by_baseclass(&pci)) {
        printk("found display pci:\n");    
        pci_device_parse_config(&pci);
        parse_pci_config(&pci);
        pci_dump_config(&pci);
        set_bars_enable(&pci,true,true);
        for (int bar_idx = 1; bar_idx <= 6; bar_idx++) {
        struct pci_bar *bar = pci_get_bar(&pci, bar_idx);

            if (bar) {
                u64 addr = pci_bar_get_addr64(bar);
		u64 size = pci_bar_get_size(bar);
                if (size>0x10000) {
                    printk("buff addr :%lx,size:%lx\n",addr,size);
                    ptr=ioremap_nocache(addr, size);
                    for (int i=0;i<100000;i++) *ptr++ = (char)i;

                }
            }
        }

    } else {
        printk("no display found\n");
    }
}
