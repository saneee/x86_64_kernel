#ifndef _DRIVER_IDE_H
#define _DRIVER_IDE_H
#define PRIMARY_MASTER  (0xe0 | (0<<4))

enum ide_register_base {
    PORT0 = 0x1f0,
    PORT1 = 0x3f6,
    PORT2 = 0x170,
    PORT3 = 0x376
};

enum ide_register {
    REG_DATA = 0x00,
    REG_ERROR = 0x01,
    REG_FEATURES = 0x01,
    REG_SECCOUNT0 = 0x02,
    REG_LBA0 = 0x03,
    REG_LBA1 = 0x04,
    REG_LBA2 = 0x05,
    REG_HDDEVSEL = 0x06,
    REG_COMMAND = 0x07,
    REG_STATUS = 0x07,
    REG_SECCOUNT1 = 0x08,
    REG_LBA3 = 0x09,
    REG_LBA4 = 0x0a,
    REG_LBA5 = 0x0b,
    REG_CONTROL = 0x0c,
    REG_ALTSTATUS = 0x0c,
    REG_DEFVADDRESS = 0x0d
};
enum ide_command {
    CMD_READ_PIO = 0x20,
    CMD_READ_PIO_EXT = 0x24,
    CMD_WRITE_PIO = 0x30,
    CMD_WRITE_PIO_EXT = 0x34,
    CMD_FLUSH = 0xe7,
    CMD_FLUSH_EXT = 0xea,
    CMD_IDENTIFY = 0xec
};

enum ide_status {
    STATUS_BSY = 0x80,
    STATUS_DRDY = 0x40,
    STATUS_DRQ = 0x08,
    STATUS_DF = 0x20,
    STATUS_ERR = 0x01
};

enum ide_control {
    CONTROL_SRST = 0x04,
    CONTROL_NIEN = 0x02
};
enum ide_identify {
    IDENTIFY_LBA_EXT = 100
};
#endif
