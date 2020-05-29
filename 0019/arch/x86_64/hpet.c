#include <yaos/types.h>
#include <yaos/compiler.h>
#include <asm/hpet.h>
#include <asm/pgtable.h>
#include <yaos/init.h>
#include <asm/apic.h>
#include <yaos/printk.h>
#include <asm/apic.h>
#include <asm/irq.h>
#include <asm/current.h>
#include <yaos/time.h>
#include <yaos/msi.h>
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
void __iomem *hpet_virt_address;
u64 hpet_period=0;

__used static void hpet_enable_legacy_int(void)
{
    unsigned int cfg = hpet_readl(HPET_CFG);

    cfg |= HPET_CFG_LEGACY;
    hpet_writel(cfg, HPET_CFG);
    hpet_legacy_int_enabled = true;
}
__used static void hpet_disable_legacy_int(void)
{
    unsigned int cfg = hpet_readl(HPET_CFG);

    cfg &= ~HPET_CFG_LEGACY;
    hpet_writel(cfg, HPET_CFG);
    hpet_legacy_int_enabled = false;
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
void hpet_msi_unmask(int idx)
{
    unsigned int cfg;

    cfg = hpet_readl(HPET_Tn_CFG(idx));
    cfg |= HPET_TN_ENABLE | HPET_TN_FSB;
    hpet_writel(cfg, HPET_Tn_CFG(idx));
}

void hpet_msi_mask(int idx)
{
    unsigned int cfg;

    cfg = hpet_readl(HPET_Tn_CFG(idx));
    cfg &= ~(HPET_TN_ENABLE | HPET_TN_FSB);
    hpet_writel(cfg, HPET_Tn_CFG(idx));
}


void hpet_msi_write(int idx, struct msi_msg *msg)
{
    hpet_writel(msg->data, HPET_Tn_ROUTE(idx));
    hpet_writel(msg->address_lo, HPET_Tn_ROUTE(idx) + 4);
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
    unsigned long cfg, cmp, now;

    hpet_virt_address = ioremap_nocache(HPET_BASE_ADDR, HPET_MMAP_SIZE);

    hpet_stop_counter();
    u64 period = hpet_readq(HPET_PERIOD);
    u64 delta64 = MSEC_PER_NSEC / (period / FSEC_PER_NSEC);

    DEBUG_PRINTK("period:%u,delta:%ld\n", period,  delta64);
    hpet_period = period / 1000000UL; //nanoseconds
    now = hpet_readq(HPET_COUNTER);
    cmp = now + delta64;
    cfg = hpet_readq(HPET_T0_CFG);
    cfg |= HPET_TN_ENABLE | HPET_TN_SETVAL;// | HPET_TN_PERIODIC;

    hpet_writeq(cfg, HPET_T0_CFG);
    hpet_writeq(cmp, HPET_T0_CMP);
    udelay(1);
    hpet_writeq(delta64, HPET_T0_CMP);

    hpet_start_counter();
    hpet_print_config();
    hpet_restart_counter();
    //hpet_enable_legacy_int();
    hpet_disable_legacy_int();
    for (int i = 0; i < 100; i++)
        ioapic_enable(i, 0);
    ioapic_enable(2, 0);
    ioapic_enable(8, 0);
    unmask_lapic_irq();
  
}
void hpet_oneshoot(int channelid, u64 delta)
{
    u64 cmp = hpet_readq(HPET_COUNTER) + delta;
    hpet_writeq(cmp, HPET_Tn_CMP(channelid));
}
void hpet_set_msi(int vector,int distid,int channelid)
{
    struct msi_msg msg;
    void msi_compose_msg(int,int,struct msi_msg *);
    msi_compose_msg(vector,distid,&msg);
    printk("msi_msg：%lx,%lx,%lx\n",msg.address_lo,msg.address_hi,msg.data);
    hpet_msi_unmask(channelid);
    hpet_msi_write(channelid,&msg);

}
static __init int init_hpet_call(bool isbp)
{
    if (isbp) {
        init_hpet();
        hpet_set_msi(45,1,2);
        hpet_oneshoot(2,100000UL*2000UL);//10seconds
        
        hpet_set_msi(46,1,3);
        hpet_oneshoot(3,100000UL*5000UL);//10seconds

    }
    return 0;
}
static void msec_callback(u64 nsec,void *param)
{
   extern u64 msec_count;
   void run_local_timers(void);
   msec_count = hpet_uptime_msec();
   //msec_count++
   set_timeout_nsec(1000000, msec_callback,param);
   run_local_timers();
}
static void msec_apcallback(u64 nsec,void *param)
{
   extern u64 msec_count;
   void run_local_timers(void);
   set_timeout_nsec(1000000, msec_apcallback,param);
   run_local_timers();
}

static __init int init_irq_call(bool isbp)
{
    if(isbp)set_timeout_nsec(1000000, msec_callback,current);
    else set_timeout_nsec(1000000, msec_apcallback,current);

    sti();
    return 0;
}
late_initcall(init_irq_call);

core_initcall(init_hpet_call);
