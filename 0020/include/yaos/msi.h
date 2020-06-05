#ifndef YAOS_MSI_H
#define YAOS_MSI_H
#include <asm/msidef.h>
#include <asm/irq.h>
struct msi_msg {
        u32     address_lo;     /* low 32 bits of msi message address */
        u32     address_hi;     /* high 32 bits of msi message address */
        u32     data;           /* 16 bits of msi message data */
};
struct pci_device;
extern bool msi_register_one(struct pci_device * pci,uint destid,irq_handler_t p);
extern void msi_compose_msg(uint vector,uint destid, struct msi_msg *msg);


#endif
