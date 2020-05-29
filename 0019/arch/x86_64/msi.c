#include <types.h>
#include <yaos/printk.h>
#include <asm/msidef.h>
#include <yaos/msi.h>
#include <asm/apic.h>
#include <asm/irq.h>
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
