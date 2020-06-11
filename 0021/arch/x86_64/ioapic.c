#include <asm/cpu.h>
#include <yaos/printk.h>
#include <asm/irq.h>
#include <asm/pgtable.h>
#define IOAPIC_BASE  0xFEC00000      // Default physical address of IO APIC

#define REG_ID     0x00         // Register index: ID
#define REG_VER    0x01         // Register index: version
#define REG_TABLE  0x10         // Redirection table base

// The redirection table starts at REG_TABLE and uses
// two registers to configure each interrupt.
// The first (low) register in a pair contains configuration bits.
// The second (high) register contains a bitmask telling which
// CPUs can serve that interrupt.
#define INT_DISABLED   0x00010000	// Interrupt disabled
#define INT_LEVEL      0x00008000	// Level-triggered (vs edge-)
#define INT_ACTIVELOW  0x00002000	// Active low (vs high)
#define INT_LOGICAL    0x00000800	// Destination is CPU id (vs APIC ID)

// IO APIC MMIO structure: write reg, then read or write data.
struct ioapic_t {
    uint reg;
    uint pad[3];
    uint data;
};

volatile struct ioapic_t *ioapic_ptr;
extern uchar ioapicid;          //acip.c

uint ioapic_read(int reg)
{
    ioapic_ptr->reg = reg;
    return ioapic_ptr->data;
}

static void ioapic_write(int reg, uint data)
{
    ioapic_ptr->reg = reg;
    ioapic_ptr->data = data;
}

void init_ioapic(void)
{
    int i, id, maxintr;

    outb(0xff, 0x21);           //mask 8259 irq
    outb(0xff, 0xa1);           //mask 8259 slave irq,use ioapic 

    ioapic_ptr = (volatile struct ioapic_t *)IO2V(IOAPIC_BASE);
    maxintr = (ioapic_read(REG_VER) >> 16) & 0xFF;
    printk("maxintr:%d\n", maxintr);
    id = ioapic_read(REG_ID) >> 24;
    if (id != ioapicid)
        printk("ioapicinit: id isn't equal to ioapicid; not a MP\n");

    // Mark all interrupts edge-triggered, active high, disabled,
    // and not routed to any CPUs.
    for (i = 0; i <= maxintr; i++) {
        ioapic_write(REG_TABLE + 2 * i, INT_DISABLED | (T_IRQ0 + i));
        ioapic_write(REG_TABLE + 2 * i + 1, 0);
    }
}

void ioapic_enable(int irq, int cpunum)
{

    // Mark interrupt edge-triggered, active high,
    // enabled, and routed to the given cpunum,
    // which happens to be that cpu's APIC ID.
    ioapic_write(REG_TABLE + 2 * irq, T_IRQ0 + irq);
    ioapic_write(REG_TABLE + 2 * irq + 1, cpunum << 24);
}
