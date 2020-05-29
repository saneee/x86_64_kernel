#include <types.h>
#include <yaos/init.h>
#include <yaos/time.h>
#include <yaos/printk.h>
#include <yaos/smp.h>
#include <yaos/cpupm.h>
#include <yaos/tasklet.h>
#include <yaos/sched.h>
#include <asm/current.h>
#include <yaoscall/malloc.h>
#include <drivers/pci_device.h>
#include <drivers/virtio.h>
#define VIRTIO_NET_SUBID 0x1000

static int test_init(bool isbp)
{

    if(isbp)return 0;
    struct pci_device pci = {._vendor_id = VIRTIO_VENDOR_ID,
                         ._device_id = VIRTIO_NET_SUBID};
    if (find_pci(&pci)){
        pci_device_parse_config(&pci);
        parse_pci_config(&pci);
        pci_dump_config(&pci);   
    }
    return 0;
}


late_initcall(test_init);

