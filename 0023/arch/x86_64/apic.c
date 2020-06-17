#include <asm/apic.h>
#include <asm/cpu.h>
#include <asm/irq.h>
#include <asm/pgtable.h>
#include <asm/bitops.h>
#include <yaos/printk.h>
#include <asm/cpu.h>
#define CMOS_PORT 0x70
#if 0
#define DEBUG_PRINT printk
#else
#define DEBUG_PRINT inline_printk
#endif

static unsigned APIC_SHORTHAND_SELF = 0x40000;

//static unsigned APIC_SHORTHAND_ALL = 0x80000;
static unsigned APIC_SHORTHAND_ALLBUTSELF = 0xC0000;
static unsigned APIC_ICR_TYPE_FIXED = 0x00000;
static unsigned APIC_ICR_LEVEL_ASSERT = 1 << 14;
static unsigned APIC_BASE_GLOBAL_ENABLE = 1 << 11;
static unsigned ICR2_DESTINATION_SHIFT = 24;
bool is_xapic = true;
bool is_x2apic = false;
u32 *lapic_base;                //set by acpi.c
struct apic_method *p_apic_func;
struct apic_method x2apic_method;
struct apic_method xapic_method;
u64 _apic_base;
static void lapic_software_enable()
{
    lapic_write(APIC_LVT0, 0);
    lapic_write(APIC_SPIV, APIC_SPIV_APIC_ENABLED | SPURIOUS_APIC_VECTOR);

}

static u32 x2apic_read(unsigned reg)
{
    return rdmsr(0x800 + reg / 0x10);
}

static void x2apic_write(unsigned reg, u32 value)
{
    wrmsr(0x800 + reg / 0x10, value);
}

static u32 x2apic_id()
{
    u32 id = x2apic_read((unsigned)(APIC_ID));

    DEBUG_PRINT("x2apic_id:%x\n", id);
    return id;
}

static void x2apic_enable()
{
    u64 msr;

    msr = rdmsr(MSR_IA32_APICBASE);
    if (msr & X2APIC_ENABLE)
        return;

    wrmsr(MSR_IA32_APICBASE, msr | X2APIC_ENABLE);
    lapic_software_enable();
}

/*
static void x2apic_disable()
{
    u64 msr = rdmsr(MSR_IA32_APICBASE);

    if (!(msr & X2APIC_ENABLE))
        return;
    wrmsr(MSR_IA32_APICBASE, msr & ~(X2APIC_ENABLE | XAPIC_ENABLE));
    wrmsr(MSR_IA32_APICBASE, msr & ~(X2APIC_ENABLE));

}
*/
static void x2apic_self_ipi(unsigned vector)
{
    wrmsr(X2APIC_SELF_IPI, vector);

}

static void x2apic_ipi(unsigned apic_id, unsigned vector)
{
    wrmsr(X2APIC_ICR, vector | ((u64) (apic_id) << 32) | APIC_ICR_LEVEL_ASSERT);
}

static void x2apic_init_ipi(unsigned apic_id, unsigned vector)
{
    wrmsr_safe(X2APIC_ICR, vector | ((u64) (apic_id) << 32));
}

static void x2apic_ipi_allbutself(unsigned vector)
{
    wrmsr(X2APIC_ICR,
          vector | APIC_SHORTHAND_ALLBUTSELF | APIC_ICR_LEVEL_ASSERT);
}

static void x2apic_nmi_allbutself()
{
    wrmsr(X2APIC_ICR, APIC_SHORTHAND_ALLBUTSELF |
          lapic_delivery(NMI_DELIVERY) | APIC_ICR_LEVEL_ASSERT);
}

static void x2apic_eoi()
{
    wrmsr(X2APIC_EOI, 0);

}

struct apic_method x2apic_method = {
    x2apic_read,
    x2apic_write,
    x2apic_id,
    x2apic_enable,
    x2apic_self_ipi,
    x2apic_ipi,
    x2apic_init_ipi,
    x2apic_ipi_allbutself,
    x2apic_nmi_allbutself,
    x2apic_eoi
};

static u32 xapic_read(unsigned reg)
{
    return lapic_base[reg / 4];
}

static void xapic_write(unsigned reg, u32 value)
{
    lapic_base[reg / 4] = value;
}

static u32 xapic_id()
{
    return xapic_read((unsigned)(APIC_ID));
}

static void xapic_enable()
{

    wrmsr(IA32_APIC_BASE, _apic_base | APIC_BASE_GLOBAL_ENABLE);
    lapic_software_enable();

}

static void xapic_nmi_allbutself()
{
    xapic_write(APIC_ICR, APIC_ICR_TYPE_FIXED | APIC_SHORTHAND_ALLBUTSELF |
                lapic_delivery(NMI_DELIVERY));

}

static void xapic_ipi_allbutself(unsigned vector)
{
    xapic_write(APIC_ICR, APIC_ICR_TYPE_FIXED | APIC_SHORTHAND_ALLBUTSELF |
                vector | APIC_ICR_LEVEL_ASSERT);
}

static void xapic_ipi(unsigned apic_id, unsigned vector)
{
    xapic_write(APIC_ICR2, apic_id << ICR2_DESTINATION_SHIFT);
    xapic_write(APIC_ICR, vector | APIC_ICR_LEVEL_ASSERT);
}

static void xapic_eoi()
{
    xapic_write(APIC_EOI, 0);
}

static void xapic_self_ipi(unsigned vector)
{
    xapic_ipi(APIC_ICR_TYPE_FIXED | APIC_SHORTHAND_SELF, vector);
}

static void xapic_init_ipi(unsigned apic_id, unsigned vector)
{
    xapic_ipi(apic_id, vector);
}

struct apic_method xapic_method = {
    xapic_read,
    xapic_write,
    xapic_id,
    xapic_enable,
    xapic_self_ipi,
    xapic_ipi,
    xapic_init_ipi,
    xapic_ipi_allbutself,
    xapic_nmi_allbutself,
    xapic_eoi
};

struct msi_message apic_compose_msix(u8 vector, u8 dest_id)
{
    struct msi_message msg = { 0, 0 };

    if (vector <= 15) {
        return msg;
    }

    msg.addr = (_apic_base & 0xFFF00000) | (dest_id << 12);

    msg.data =
        (FIXED_DELIVERY << MSI_DELIVERY_MODE) |
        (MSI_ASSSERT << MSI_LEVEL_ASSERTION) |
        (TRIGGER_MODE_EDGE << MSI_TRIGGER_MODE) | vector;

    return msg;
}

void probe_apic()
{
    u32 eax = 1;
    u32 ecx = 0;
    u32 ebx, edx;

    _apic_base = lapic_read_base();
    DEBUG_PRINT("_apic_base:%lx\n\n", _apic_base);
    native_cpuid(&eax, &ebx, &ecx, &edx);
    if (test_bit(9, (ulong *) & edx)) {
        is_xapic = true;
        DEBUG_PRINT("xAPIC ok %lx %lx %lx %lx\n", eax, ebx, ecx, edx);
    }
    if (test_bit(21, (ulong *) & ecx)) {
        is_x2apic = true;
        DEBUG_PRINT("x2APIC ok\n");
        p_apic_func = &x2apic_method;
    }
    else {
        p_apic_func = &xapic_method;
    }
    eax = 0;
    native_cpuid(&eax, &ebx, &ecx, &edx);
    DEBUG_PRINT("Of:%lx,%lx,%lx,%lx\n", eax, ebx, ecx, edx);
    eax = 7;
    native_cpuid(&eax, &ebx, &ecx, &edx);
    write_cr4(read_cr4() | cr4_fsgsbase);
    DEBUG_PRINT("O7:%lx,%lx,%lx,%lx,cr4:%lx\n", eax, ebx, ecx, edx, read_cr4());

    eax = 0x0b;
    ecx = 0;
    native_cpuid(&eax, &ebx, &ecx, &edx);
    DEBUG_PRINT("Obf:%lx,%lx,%lx,%lx\n", eax, ebx, ecx, edx);
}

void lapic_start_ap(uint apicid, uint addr)
{
    lapic_init_ipi(apicid, 0x4500);
    lapic_init_ipi(apicid, 0x4600 | (addr >> 12));
    lapic_init_ipi(apicid, 0x4600 | (addr >> 12));

}

void init_lapic(void)
{
    probe_apic();
    lapic_enable();
    DEBUG_PRINT("\n\napicid:%lx,%lx\n", lapic_id(), cpuid_apic_id());
}

void init_lapic2(void)
{
    probe_apic();
    lapic_enable();
    // Enable local APIC; set spurious interrupt vector.
    lapic_write(APIC_SPIV, APIC_SPIV_APIC_ENABLED | SPURIOUS_APIC_VECTOR);
    // The timer repeatedly counts down at bus frequency
    // from lapic[TICR] and then issues an interrupt.
    // If xv6 cared more about precise timekeeping,
    // TICR would be calibrated using an external time source.
    lapic_write(APIC_TDCR, APIC_TDR_DIV_1);
    lapic_write(APIC_LVTT, APIC_LVT_TIMER_PERIODIC | LOCAL_TIMER_VECTOR);
    lapic_write(APIC_TMICT, 10000000);

    // Disable logical interrupt lines.
    lapic_write(APIC_LVT0, APIC_LVT_MASKED);
    lapic_write(APIC_LVT1, APIC_LVT_MASKED);

// Disable performance counter overflow interrupts
    // on machines that provide that interrupt entry.
    if (GET_APIC_MAXLVT(lapic_read(APIC_LVR) >= 4)) {
        lapic_write(APIC_LVTPC, APIC_LVT_MASKED);
    }

    // Map error interrpic_write(APIC_SPIV, APIC_SPIV_APIC_ENABLED | SPURIOUS_APIC_VECTOR);
    lapic_write(APIC_SPIV, APIC_SPIV_APIC_ENABLED | SPURIOUS_APIC_VECTOR);
    lapic_write(APIC_LVTERR, ERROR_APIC_VECTOR);

    // Clear error status register (requires back-to-back writes).
    lapic_write(APIC_ESR, 0);
    lapic_write(APIC_ESR, 0);

    // Ack any outstanding interrupts.
    lapic_write(APIC_EOI, 0);

    // Send an Init Level De-Assert to synchronise arbitration ID's.
    lapic_write(APIC_ICR2, 0);
    lapic_write(APIC_ICR, APIC_DEST_ALLINC | APIC_DM_INIT | APIC_INT_LEVELTRIG);
    while (lapic_read(APIC_ICR) & APIC_ICR_BUSY) {
    }

    lapic_write(APIC_TASKPRI, 0);
}
