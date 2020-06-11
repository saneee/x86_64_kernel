#include <types.h>
#include <yaos/printk.h>
#include <asm/msidef.h>
#include <yaos/msi.h>
#include <asm/apic.h>
#include <asm/irq.h>
#include <drivers/pci_device.h>
static void __irq_msi_compose_msg(struct irq_cfg *cfg, struct msi_msg *msg)
{
    msg->address_hi = MSI_ADDR_BASE_HI;

    if (x2apic_enabled())
        msg->address_hi |= MSI_ADDR_EXT_DEST_ID(cfg->dest_apicid);

    msg->address_lo =
        MSI_ADDR_BASE_LO |
        MSI_ADDR_DEST_MODE_PHYSICAL|
        MSI_ADDR_REDIRECTION_CPU |
        MSI_ADDR_DEST_ID(cfg->dest_apicid);

   msg->data =
       MSI_DATA_TRIGGER_EDGE |
       MSI_DATA_LEVEL_ASSERT |
       MSI_DATA_DELIVERY_FIXED |
       MSI_DATA_VECTOR(cfg->vector);
}
void msi_compose_msg(uint vector,uint destid, struct msi_msg *msg)
{
    struct irq_cfg cfg={.vector = vector,.dest_apicid = destid};
    __irq_msi_compose_msg(&cfg,msg);
}
bool msi_register_one(pci_device_t pci,uint destid,irq_handler_t p)
{
    uint n = register_irq_handler(p);
    if (!n) return false;
    pci_d("register_irq_handler:%d,handler:%p\n",n,p);
    if (pci_is_msix(pci)) {
        pci_msix_enable(pci);
    } else {
        pci_msi_enable(pci);
    }
    struct msi_msg msg;
    msi_compose_msg(n,destid,&msg);

    if (pci_is_msix(pci)) {
        msix_mask_entry(pci,0);
        pci_msix_write_entry(pci, 0, &msg);
        msix_unmask_entry(pci,0);

    } else {
        msi_mask_entry(pci,0);
        pci_msi_write_entry(pci, 0, &msg);
        msi_unmask_entry(pci,0);
    }    
    return true;
}
bool msi_register_arr(pci_device_t pci,uint destid,int nr,irq_handler_t * p)
{
    if (pci_is_msix(pci)) {
        pci_msix_enable(pci);
    } else {
        pci_msi_enable(pci);
    }

    for (int i=0;i<nr;i++) {
        uint n = register_irq_handler(p[i]);
        if (!n) return false;
        pci_d("register_irq_handler:%d,pos:%d,nr:%d,handler:%p\n",n,i,nr,p[i]);
        struct msi_msg msg;
        msi_compose_msg(n,destid,&msg);

        if (pci_is_msix(pci)) {
            msix_mask_entry(pci,i);
            pci_msix_write_entry(pci, i, &msg);
            msix_unmask_entry(pci,i);

        } else {
            msi_mask_entry(pci,i);
            pci_msi_write_entry(pci, i, &msg);
            msi_unmask_entry(pci,i);
        }
    }
    return true;
}

