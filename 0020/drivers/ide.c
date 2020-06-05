#include <types.h>
#include <yaos/printk.h>
#include "pci_function.h"
#include "pci_device.h"
#include <yaos/init.h>
#include <yaos/irq.h>
#include "ide.h"
#include <yaos/sched.h>
#define SECTOR_SIZE 512
#define BIO_READ 1
#define BIO_WRITE 2
#define BIO_FLUSH 4
static ulong hdasize=0;
static inline void reg_writeb(int base, int reg, u8 val)
{
    outb(val, base + reg);
}
static inline u8 reg_readb(int base, int reg)
{
    return inb(base + reg);
}
static inline void reg_readsl(int base, int reg, void *buf, int cnt)
{
    insl(buf, cnt, base + reg);
}
static inline void reg_writesl(int base, int reg, void *buf, int cnt)
{
    outsl(buf, cnt, base + reg);
}
static int ide_poll()
{
    int status;
    u8 mask = (STATUS_BSY|STATUS_DRDY);
    u8 match = STATUS_DRDY;
    while (((status = reg_readb(PORT0, REG_STATUS)) & mask) != match)
        nsleep(4000);
    reg_readb(PORT0, REG_ALTSTATUS);
    return status;
}
int64_t ide_size()
{
    unsigned short buf[256];
    int64_t *sectors;
    ide_poll();
    reg_writeb(PORT0, REG_COMMAND, CMD_IDENTIFY);
    ide_poll();
    reg_readsl(PORT0, REG_DATA, buf, sizeof(buf)/4);
    sectors = (int64_t *)(&buf[IDENTIFY_LBA_EXT]);
    return *sectors * SECTOR_SIZE;

}
void ide_io(uint8_t cmd, u32 sector, void *data, u8 cnt, bool head)
{
    if (head) {
        ide_poll();
        reg_writeb(PORT0, REG_SECCOUNT0, cnt);
        reg_writeb(PORT0, REG_LBA0, sector & 0xff);
        reg_writeb(PORT0, REG_LBA1, (sector >> 8) & 0xff);
        reg_writeb(PORT0, REG_LBA2, (sector >> 16) & 0xff);
        /* Select primary master */
        reg_writeb(PORT0, REG_HDDEVSEL,
            PRIMARY_MASTER | ((sector >> 24) & 0x0f));
    }
    switch (cmd) {
    case BIO_READ:
        if (head)
            reg_writeb(PORT0, REG_COMMAND, CMD_READ_PIO);
        ide_poll();
        reg_readsl(PORT0, REG_DATA, data, SECTOR_SIZE/4);
        break;
    case BIO_WRITE:
        if (head)
            reg_writeb(PORT0, REG_COMMAND, CMD_WRITE_PIO);
        reg_writesl(PORT0, REG_DATA, data, SECTOR_SIZE/4);
        ide_poll();
        break;
    case BIO_FLUSH:
        if (head) {
            reg_writeb(PORT0, REG_COMMAND, CMD_FLUSH);
            ide_poll();
        }
        break;
    default:
        panic("Invalid cmd");
    }
}
void ide_read(u32 sector, void *data, u8 cnt, bool head)
{
    if(hdasize){
        ide_io(BIO_READ, sector,data,cnt,head);
        reg_readb(PORT0, REG_STATUS);
        reg_readb(PORT0, REG_ALTSTATUS);

    }
    else printk("no hda exist\n");
}
void ide_write(u32 sector, void *data, u8 cnt, bool head)
{
    if(hdasize){
        ide_io(BIO_WRITE, sector,data,cnt,head);
        reg_readb(PORT0, REG_STATUS);
        reg_readb(PORT0, REG_ALTSTATUS);
    }
    else printk("no hda exist\n");
}

static void ide_probe()
{
    struct pci_device pci = {._base_class_code = PCI_CLASS_STORAGE,
       ._sub_class_code = PCI_SUB_CLASS_STORAGE_IDE};
    if (find_pci_by_subclass(&pci)) {
        printk("\n\n\nfound ide pci:\n");
        pci_device_parse_config(&pci);
        parse_pci_config(&pci);
        local_irq_disable();
        pci_dump_config(&pci);
        local_irq_enable();
        /* Reset controller */
        reg_writeb(PORT0, REG_CONTROL, CONTROL_SRST);
        msleep(2);
        reg_writeb(PORT0, REG_CONTROL, 0);
        nsleep(400);

        /* Select primary master */
        reg_writeb(PORT0, REG_HDDEVSEL, PRIMARY_MASTER);
        u8 sel = reg_readb(PORT0, REG_HDDEVSEL);
        if (sel != PRIMARY_MASTER) {/* No drive */
            printk("no master drive\n");
            return;
        }

        /* Disable intterupt for primary master/slave */
        reg_writeb(PORT0, REG_CONTROL, CONTROL_NIEN);
        reg_writeb(PORT1, REG_CONTROL, CONTROL_NIEN);

        /* Send CMD_IDENTIFY */
        reg_writeb(PORT0, REG_COMMAND, CMD_IDENTIFY);
        printk("ide master drive size:%ld\n",(hdasize=ide_size()));
        return ;

    } else printk("\n\n\nno ide pci found\n");
}
static int ide_init(bool isbp)
{
    if(isbp){
        ide_probe();
    }
    return 0;
}


device_initcall(ide_init);

