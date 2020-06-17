#include <pci_def.h>
#include <asm/cpu.h>
#include <yaos/printk.h>
#include <yaos/types.h>
#include <yaos/queue.h>
#include <asm/pci.h>
#include <drivers/pci_device.h>
#include <yaos/init.h>
#define MAX_PCI    128
struct pci_bsf {
    uint16_t vid;
    uint16_t did;
    uint8_t b;
    uint8_t s;
    uint8_t f;
};
static int nr_pci;
struct pci_bsf pci_bsf[MAX_PCI];
bool find_pci(pci_device_t pci)
{
    for (int i = 0; i < nr_pci; i++) {
        if (pci_bsf[i].vid == pci->_vendor_id
            && pci_bsf[i].did == pci->_device_id) {
            pci->_bus = pci_bsf[i].b;
            pci->_device = pci_bsf[i].s;
            pci->_func = pci_bsf[i].f;
            return true;
        }
    }
    return false;
}
bool find_pci_by_baseclass(pci_device_t pci)
{
    uchar baseclass,subclass;
    for (int i = 0; i < nr_pci; i++) {
       baseclass = read_pci_config_byte(pci_bsf[i].b, pci_bsf[i].s, pci_bsf[i].f, PCI_CFG_CLASS_CODE0);
       subclass  = read_pci_config_byte(pci_bsf[i].b, pci_bsf[i].s, pci_bsf[i].f, PCI_CFG_CLASS_CODE1);

       if (baseclass == pci->_base_class_code && subclass == pci->_sub_class_code) {
           pci->_bus = pci_bsf[i].b;
           pci->_device = pci_bsf[i].s;
           pci->_func = pci_bsf[i].f;
           return true;
       }
    }
    return false;
}
bool find_pci_by_subclass(pci_device_t pci)
{
    uchar baseclass;
    for (int i = 0; i < nr_pci; i++) {
       baseclass = read_pci_config_byte(pci_bsf[i].b, pci_bsf[i].s, pci_bsf[i].f, PCI_CFG_CLASS_CODE0);
       if (baseclass == pci->_base_class_code) {
           pci->_bus = pci_bsf[i].b;
           pci->_device = pci_bsf[i].s;
           pci->_func = pci_bsf[i].f;
           return true;
       }
    }
    return false;
}

void __init init_pci()
{
    nr_pci = 0;
    for (int b = 0; b <= 255; b++) {
        for (int s = 0; s <= 31; s++) {
            for (int f = 0; f <= 7; f++) {
                uint16_t vid = read_pci_config_word(b, s, f, 0);
                uint16_t did = read_pci_config_word(b, s, f, 2);

                if (vid != 0xffff) {
                    pci_bsf[nr_pci].vid = vid;
                    pci_bsf[nr_pci].did = did;
                    pci_bsf[nr_pci].b = b;
                    pci_bsf[nr_pci].s = s;
                    pci_bsf[nr_pci].f = f;
                    nr_pci++;
                    printf("b:%d,s:%d,f:%d,vid:%lx,%lx\n", b, s, f, vid, did);

                }
            }
        }
    }
}
