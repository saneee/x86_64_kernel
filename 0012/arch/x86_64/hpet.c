#include <yaos/types.h>
#include <yaos/compiler.h>
#include <asm/hpet.h>
#include <asm/pgtable.h>
#include <yaos/init.h>
#include <asm/apic.h>
#include <yaos/printk.h>
#include <asm/apic.h>
#include <asm/irq.h>
#if 1
#define DEBUG_PRINTK printk
#else
#define DEBUG_PRINTK inline_printk
#endif

#define HPET_MASK			CLOCKSOURCE_MASK(32)

/* FSEC = 10^-15
   NSEC = 10^-9 */
#define FSEC_PER_NSEC			1000000L

#define HPET_DEV_USED_BIT		2
#define HPET_DEV_USED			(1 << HPET_DEV_USED_BIT)
#define HPET_DEV_VALID			0x8
#define HPET_DEV_FSB_CAP		0x1000
#define HPET_DEV_PERI_CAP		0x2000

#define HPET_MIN_CYCLES			128
#define HPET_MIN_PROG_DELTA		(HPET_MIN_CYCLES + (HPET_MIN_CYCLES >> 1))

#define HPET_BASE_ADDR 0xfed00000
/*
 * HPET address is set in acpi/boot.c, when an ACPI entry exists
 */
/* FSEC = 10^-15
   NSEC = 10^-9 
   MSEC = 10^-3
*/
#define FSEC_PER_NSEC                   1000000L
#define MSEC_PER_NSEC                   1000000L
bool hpet_verbose = true;
bool hpet_legacy_int_enabled = false;
static void __iomem *hpet_virt_address;

static inline unsigned int hpet_readl(unsigned int a)
{
    return (*(volatile u32 *)(hpet_virt_address + a));
}

static inline void hpet_writel(unsigned int d, unsigned int a)
{
    *(volatile u32 *)(hpet_virt_address + a) = d;

}

static void hpet_enable_legacy_int(void)
{
    unsigned int cfg = hpet_readl(HPET_CFG);

    cfg |= HPET_CFG_LEGACY;
    hpet_writel(cfg, HPET_CFG);
    hpet_legacy_int_enabled = true;
}

static void hpet_stop_counter(void)
{
    unsigned long cfg = hpet_readl(HPET_CFG);

    cfg &= ~HPET_CFG_ENABLE;
    hpet_writel(cfg, HPET_CFG);
}

static void hpet_reset_counter(void)
{
    hpet_writel(0, HPET_COUNTER);
    hpet_writel(0, HPET_COUNTER + 4);
}

static void hpet_start_counter(void)
{
    unsigned int cfg = hpet_readl(HPET_CFG);

    cfg |= HPET_CFG_ENABLE;
    hpet_writel(cfg, HPET_CFG);
}

static void hpet_restart_counter(void)
{
    hpet_stop_counter();
    hpet_reset_counter();
    hpet_start_counter();
}

static void _hpet_print_config(const char *function, int line)
{
    u32 i, timers, l, h;

    DEBUG_PRINTK("hpet: %s(%d):\n", function, line);
    l = hpet_readl(HPET_ID);
    h = hpet_readl(HPET_PERIOD);
    timers = ((l & HPET_ID_NUMBER) >> HPET_ID_NUMBER_SHIFT) + 1;
    DEBUG_PRINTK("hpet: ID: 0x%x, PERIOD: 0x%x\n", l, h);
    l = hpet_readl(HPET_CFG);
    h = hpet_readl(HPET_STATUS);
    DEBUG_PRINTK("hpet: CFG: 0x%x, STATUS: 0x%x\n", l, h);
    l = hpet_readl(HPET_COUNTER);
    h = hpet_readl(HPET_COUNTER + 4);
    DEBUG_PRINTK("hpet: COUNTER_l: 0x%x, COUNTER_h: 0x%x\n", l, h);

    for (i = 0; i < timers; i++) {
        l = hpet_readl(HPET_Tn_CFG(i));
        h = hpet_readl(HPET_Tn_CFG(i) + 4);
        DEBUG_PRINTK("hpet: T%d: CFG_l: 0x%x, CFG_h: 0x%x\n", i, l, h);
        l = hpet_readl(HPET_Tn_CMP(i));
        h = hpet_readl(HPET_Tn_CMP(i) + 4);
        DEBUG_PRINTK("hpet: T%d: CMP_l: 0x%x, CMP_h: 0x%x\n", i, l, h);
        l = hpet_readl(HPET_Tn_ROUTE(i));
        h = hpet_readl(HPET_Tn_ROUTE(i) + 4);
        DEBUG_PRINTK("hpet: T%d ROUTE_l: 0x%x, ROUTE_h: 0x%x\n", i, l, h);
    }
}

#define hpet_print_config()                                     \
do {                                                            \
        if (hpet_verbose)                                       \
                _hpet_print_config(__func__, __LINE__); \
} while (0)

static void udelay()
{

}

void __init init_hpet(void)
{
    unsigned int cfg, cmp, now;

    hpet_virt_address = ioremap_nocache(HPET_BASE_ADDR, HPET_MMAP_SIZE);

    hpet_stop_counter();
    u32 period = hpet_readl(HPET_PERIOD);
    u64 delta64 = MSEC_PER_NSEC / (period / FSEC_PER_NSEC);
    u32 delta = (u32) delta64;

    DEBUG_PRINTK("period:%u,delta:%d,%ld\n", period, delta, delta64);

    now = hpet_readl(HPET_COUNTER);
    cmp = now + delta;
    cfg = hpet_readl(HPET_T0_CFG);
    cfg |= HPET_TN_ENABLE | HPET_TN_PERIODIC | HPET_TN_SETVAL | HPET_TN_32BIT;

    hpet_writel(cfg, HPET_T0_CFG);
    hpet_writel(cmp, HPET_T0_CMP);
    udelay(1);
    hpet_writel(delta, HPET_T0_CMP);
    hpet_start_counter();
    hpet_print_config();
    hpet_restart_counter();
    hpet_enable_legacy_int();
    for (int i = 0; i < 100; i++)
        ioapic_enable(i, 0);
    ioapic_enable(2, 0);
    unmask_lapic_irq();
}

static __init int init_hpet_call(bool isbp)
{
    if (isbp)
        init_hpet();
    else{
        lapic_write(APIC_SPIV, APIC_SPIV_APIC_ENABLED | SPURIOUS_APIC_VECTOR);
        lapic_write(APIC_TDCR, APIC_TDR_DIV_1);
        lapic_write(APIC_LVTT, APIC_LVT_TIMER_PERIODIC | LOCAL_TIMER_VECTOR);
        lapic_write(APIC_TMICT, 10000000);
    }
    return 0;
}
static __init int init_irq_call(bool isbp)
{
    sti();
    return 0;
}
late_initcall(init_irq_call);

core_initcall(init_hpet_call);
