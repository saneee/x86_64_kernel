#ifndef _ASM_APIC_H
#define _ASM_APIC_H
#include <asm/cpu.h>
#include <asm/apicdef.h>
#include <asm/msrdef.h>
#include <yaos/types.h>
struct msi_message {
    u64 addr;
    u32 data;
};
struct apic_method {
    u32(*read) (unsigned reg);
    void (*write) (unsigned reg, u32 value);
     u32(*id) ();
    void (*enable) ();
    void (*self_ipi) (unsigned vector);
    void (*ipi) (unsigned apicid, unsigned vector);
    void (*init_ipi) (unsigned apicid, unsigned vector);
    void (*ipi_allbutself) (unsigned vector);
    void (*nmi_allbutself) (unsigned vector);
    void (*eio) ();

};
extern struct apic_method *p_apic_func;

static inline u32 lapic_id()
{
    return p_apic_func->id();
}

static inline u32 lapic_read(unsigned reg)
{
    return p_apic_func->read(reg);
}

static inline void lapic_write(unsigned reg, u32 val)
{
    return p_apic_func->write(reg, val);
}

static inline void lapic_nmi_allbutself(unsigned vector)
{
    return p_apic_func->nmi_allbutself(vector);
}

static inline void lapic_eoi()
{
    return p_apic_func->eio();
}

static inline void lapic_ipi_allbutself(unsigned vector)
{
    return p_apic_func->ipi_allbutself(vector);
}

static inline void lapic_init_ipi(unsigned apicid, unsigned vector)
{
    return p_apic_func->init_ipi(apicid, vector);
}

static inline void lapic_ipi(unsigned apicid, unsigned vector)
{
    return p_apic_func->ipi(apicid, vector);
}

static inline void lapic_enable()
{
    return p_apic_func->enable();
}

static inline void lapic_self_ipi(unsigned vector)
{
    return p_apic_func->self_ipi(vector);
}

static inline u64 lapic_read_base()
{
    return rdmsr(MSR_IA32_APICBASE) & 0xFFFFFF000;
}

static inline u32 lapic_delivery(u32 mode)
{
    return mode << DELIVERY_SHIFT;
}

static inline void mask_lapic_irq()
{
    unsigned long v;

    v = lapic_read(APIC_LVT0);
    lapic_write(APIC_LVT0, v | APIC_LVT_MASKED);
}

static inline void unmask_lapic_irq()
{
    unsigned long v;

    v = lapic_read(APIC_LVT0);
    lapic_write(APIC_LVT0, v & ~APIC_LVT_MASKED);
}

static inline void ack_lapic_irq(void)
{
    /*
     * ack_APIC_irq() actually gets compiled as a single instruction
     * ... yummie.
     */
    lapic_eoi();
}
/*
 * Get the LAPIC version
 */
static inline int lapic_get_version(void)
{
    return GET_APIC_VERSION(lapic_read(APIC_LVR));
}

/*
 * Check, if the APIC is integrated or a separate chip
 */
static inline int lapic_is_integrated(void)
{
    return APIC_INTEGRATED(lapic_get_version());
}

extern void ioapic_enable(int irq, u32 apicid);
extern struct msi_message apic_compose_msix(u8 vector, u8 dest_id);
#endif
