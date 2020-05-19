/*
 * Copyright (C) 2013 Cloudius Systems, Ltd.
 *
 * This work is open source software, licensed under the terms of the
 * BSD license as described in the LICENSE file in the top-level directory.
 */

#ifndef _PCI_DEF_H
#define _PCI_DEF_H

enum pc_early_defines {
    PCI_VENDOR_ID = 0x0,
    PCI_CONFIG_ADDRESS = 0xcf8,
    PCI_CONFIG_DATA = 0xcfc,
    PCI_BUS_OFFSET = 16,
    PCI_SLOT_OFFSET = 11,
    PCI_FUNC_OFFSET = 8,
    PCI_CONFIG_ADDRESS_ENABLE = 0x80000000,
    PCI_COMMAND_OFFSET = 0x4,
    PCI_BUS_MASTER_BIT = 0x2,
    PCI_STATUS_OFFSET = 0x6,
    PCI_CLASS_REVISION = 0x8,
    PCI_CLASS_OFFSET = 0xb,
    PCI_SUBCLASS_OFFSET = 0xa,
    PCI_HEADER_TYPE = 0xe,
    PCI_SUBSYSTEM_ID = 0x2e,
    PCI_SUBSYSTEM_VID = 0x2c,
    PCI_HEADER_MULTI_FUNC = 0x80,
    PCI_BAR0_ADDR = 0x10,
    PCI_CONFIG_SECONDARY_BUS = 0x19,
    PCI_CAPABILITIES_PTR = 0x34,
};


#endif /* _PCI_DEF_H */
