#include <asm/acpi.h>
#include <asm/pgtable.h>
#include <yaos/printk.h>
#include <yaos/string.h>
#include <asm/cpu.h>
#include <yaos/assert.h>
extern u32 *lapic_base;
int ismp;
extern int nr_cpu;
uchar ioapicid;

static struct acpi_rdsp *scan_rdsp(uint base, uint len)
{
    uchar *p;
    printk("base:%lx,size:%x,IO2V:%lx\n",base,len,P2V(base));
    for (p = (uchar *) P2V(base); len >= sizeof(struct acpi_rdsp);
         len -= 4, p += 4) {
        if (memcmp(p, SIG_RDSP, 8) == 0) {
            uint sum, n;

            for (sum = 0, n = 0; n < 20; n++)
                sum += p[n];
            if ((sum & 0xff) == 0) {
		printk("found acpi_rdsp:%lx\n",p);
		return (struct acpi_rdsp *)p;
	    }
	
        }
    }
    return (struct acpi_rdsp *)0;
}

static struct acpi_rdsp *find_rdsp(void)
{
    struct acpi_rdsp *rdsp;
    uintp pa;

    pa = *((ushort *) P2V(0x40E)) << 4;	// EBDA
    if (pa && (rdsp = scan_rdsp(pa, 1024))){
        printk("pa:%lx,rdsp:%lx\n",rdsp);
        return rdsp;
    }	
    return scan_rdsp(0xE0000, 0x20000);
}

static int acpi_config_smp(struct acpi_madt *madt)
{
    uint32 lapic_addr;
    uint nioapic = 0;
    uchar *p, *e;
    void init_cpu(u32 apicid);

    if (!madt)
        return -1;
    if (madt->header.length < sizeof(struct acpi_madt))
        return -1;

    lapic_addr = madt->lapic_addr_phys;
    printk("lapic_addr:%lx\n", lapic_addr);
    p = madt->table;
    e = p + madt->header.length - sizeof(struct acpi_madt);

    while (p < e) {
        uint len;

        if ((e - p) < 2)
            break;
        len = p[1];
        if ((e - p) < len)
            break;
        switch (p[0]) {
        case TYPE_LAPIC:{
                struct madt_lapic *lapic = (void *)p;

                if (len < sizeof(*lapic))
                    break;
                if (!(lapic->flags & APIC_LAPIC_ENABLED))
                    break;
                printk("acpi: cpu#%d apicid %d\n", nr_cpu, lapic->apic_id);
                init_cpu(lapic->apic_id);
                break;
            }
        case TYPE_IOAPIC:{
                struct madt_ioapic *ioapic = (void *)p;

                if (len < sizeof(*ioapic))
                    break;
                printk("acpi: ioapic#%d @%x id=%d base=%d\n",
                       nioapic, ioapic->addr, ioapic->id,
                       ioapic->interrupt_base);
                if (nioapic) {
                    printk("warning: multiple ioapics are not supported");
                }
                else {
                    ioapicid = ioapic->id;
                }
                nioapic++;
                break;
            }

        }
        p += len;
    }

    if (nr_cpu) {
        ismp = 1;
        lapic_base = (u32 *) IO2V(((uintp) lapic_addr));
        printk("lapic:%lx\n", (ulong) lapic_base);
        return 0;
    }

    return -1;
}

int init_acpi(void)
{
    unsigned n, count;
    struct acpi_rdsp *rdsp;
    struct acpi_rsdt *rsdt;
    struct acpi_madt *madt = 0;
    u64 get_phy_addr(u64 vaddr);

    rdsp = find_rdsp();
    if (!rdsp) {
        panic("can't found acpi rdsp\n");
    }
    printk("rdsp:%lx,%lx\n",  rdsp, rdsp->rsdt_addr_phys);
    rsdt = (struct acpi_rsdt *)IO2V(rdsp->rsdt_addr_phys);
    dump_mem(rsdt,0x100);    
    printk("rsdt:%lx,at phyaddr:%lx\n", (ulong) rsdt,get_phy_addr((ulong)rsdt));

    count = (rsdt->header.length - sizeof(*rsdt)) / 4;
    printk("count:%d\n", count);

    for (n = 0; n < count; n++) {
        struct acpi_desc_header *hdr =
            (struct acpi_desc_header *)IO2V(rsdt->entry[n]);
        printk("entry n:%d,%lx,%lx\n", n, rsdt->entry[n], (ulong) hdr);
#if DEBUG
        uchar sig[5], id[7], tableid[9], creator[5];

        memmove(sig, hdr->signature, 4);
        sig[4] = 0;
        memmove(id, hdr->oem_id, 6);
        id[6] = 0;
        memmove(tableid, hdr->oem_tableid, 8);
        tableid[8] = 0;
        memmove(creator, hdr->creator_id, 4);
        creator[4] = 0;
        printk("acpi: %s %s %s %x %s %x\n",
               sig, id, tableid, hdr->oem_revision,
               creator, hdr->creator_revision);
#endif
        if (!memcmp(hdr->signature, SIG_MADT, 4))
            madt = (void *)hdr;
    }

    return acpi_config_smp(madt);

}
