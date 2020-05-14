/*zhuhanjun 2015/11/03 Yiwu Saneee Network Co.,LTD.
*/
#include <asm/cpu.h>
#include <asm/pm64.h>
//#include "yaos.h"
#include <yaos/printk.h>
#include <asm/pgtable.h>
#include <yaos/kheap.h>
#include <yaos/errno.h>
#include <asm/cpu.h>
#define IOAPIC_BASE  0xFEC00000      // Default physical address of IO APIC


#define MAX_IO_REMAPS 2000
#if 1
#define DEBUG_PRINT printk
#else
#define DEBUG_PRINT inline_prink
#endif
struct mboot_info {
    u32 mboot_magic;
    u32 mboot_flags;
    u32 mboot_check;
    u32 mboot_head_addr;
    u32 mboot_load_addr;
    u32 mboot_load_end;
    u32 mboot_bss_end;
    u32 mboot_entry_addr;
};
struct multiboot_info_type {
    u32 flags;
    u32 mem_lower;
    u32 mem_upper;
    u32 boot_device;
    u32 cmdline;
    u32 mods_count;
    u32 mods_addr;
    u32 syms[4];
    u32 mmap_length;
    u32 mmap_addr;
    u32 drives_length;
    u32 drives_addr;
    u32 config_table;
    u32 boot_loader_name;
    u32 apm_table;
    u32 vbe_control_info;
    u32 vbe_mode_info;
    u16 vbe_mode;
    u16 vbe_interface_seg;
    u16 vbe_interface_off;
    u16 vbe_interface_len;
} __attribute__ ((packed));

#define MULTIBOOT_BOOTLOADER_MAGIC      0x2BADB002

extern void uart_early_init();
ulong __kernel_start;
ulong __kernel_end;
extern char _percpu_start[], _percpu_end[];
ulong __max_phy_addr;           //include io memory addr
ulong __max_phy_mem_addr;       //only useable memory
ulong __max_linear_addr;
ulong __percpu_size;
ulong __percpu_start;

void init_bss()
{
    //struct mboot_info *ptr;
    const ulong init_heap_size = 0x10000;	//64k

    //ptr = (struct mboot_info *)0x100000;
    __kernel_end = (ulong)_percpu_end;//ptr->mboot_bss_end;
    __percpu_size = (ulong) (_percpu_end - _percpu_start);
    __percpu_start = (ulong) _percpu_start;
    free_kheap_4k(__kernel_end, init_heap_size);
    __kernel_end += init_heap_size;
    __kernel_start = 0x100000;  //1M
printk("kstart:%lx,kend:%lx,pcpu:%lx,pcpuend:%lx\n",__kernel_start,__kernel_end,__percpu_start,__percpu_start+__percpu_size);
}

#define CHECK_FLAG(flags,bit)   ((flags) & (1 << (bit)))

typedef struct memory_map {
    u32 size;
    u32 base_addr_low;
    u32 base_addr_high;
    u32 length_low;
    u32 length_high;
    u32 type;
} memory_map_t;
typedef struct module {
    u32 mod_start;
    u32 mod_end;
    u32 string;
    u32 reserved;
} module_t;
typedef struct elf_section_header_table {
    u32 num;
    u32 size;
    u32 addr;
    u32 shndx;
} elf_section_header_table_t;
static long io_remap_pages[MAX_IO_REMAPS + 1];
static int remaped = 0;

static void add_map(int pg)
{
    for (int i = 0; i < remaped; i++) {
        if (io_remap_pages[i] == pg) {
            DEBUG_PRINT("add iomap but exist:%d\n", pg);
            return;
        }
    }
    if (remaped >= MAX_IO_REMAPS) {
        panic("MAX_IO_REMAPS too small\n");
    }
    io_remap_pages[remaped++] = pg;
    DEBUG_PRINT("add new iomap:%d\n", pg);
}

static void remap_iomem(void)
{
    for (int i = 0; i < remaped; i++) {
        ulong pg = io_remap_pages[i];

        if (pg > 0) {
            pg--;               //page index from 0
            //do not cache io memory 
            if (OK !=
                map_page_p2v(pg * PAGE_SIZE, pg * PAGE_SIZE + IO_MEM_BASE,
                             PTE_P | PTE_W | PTE_PWT | PTE_PCD | PTE_PS)){
                panic("No memory when map page %lx\n",pg*PAGE_SIZE);
}
printk("remap:IO:%lx\n",pg*PAGE_SIZE);
        }
    }
}

static void on_iomem(u64 addr, u64 size)
{
    ulong low = addr & (PAGE_SIZE - 1);

    if (!addr)
        return;
    if (low) {
        ulong lowsize = PAGE_SIZE - low;
        long pg;

        if (lowsize >= size) {
            pg = (addr + PAGE_SIZE - 1) / PAGE_SIZE;
            add_map(pg);
            return;
        }
        size -= lowsize;
        pg = (addr + PAGE_SIZE - 1) / PAGE_SIZE;
        add_map(pg);

        addr += lowsize;
        on_iomem(addr, size);
        return;

    }
    while (size >= PAGE_SIZE) {
        add_map(addr / PAGE_SIZE + 1);
        addr += PAGE_SIZE;
        size -= PAGE_SIZE;
    }
    if (size == 0)
        return;
    add_map(addr / PAGE_SIZE + 1);

}

struct reserve_addr {
    u64 addr;
    u64 size;
};

#define MAX_REVERSE 10
struct reserve_addr revmem[MAX_REVERSE + 1];
int reserved_nr = 0;
ulong max_reserve_addr = 0;
void add_reserve(u64 addr, u64 size)
{
    if (reserved_nr >= MAX_REVERSE)
        panic("MAX_REVERSE too small");
    revmem[reserved_nr].addr = addr;
    revmem[reserved_nr++].size = size;
    if (addr + size > max_reserve_addr)
        max_reserve_addr = addr + size;
}

static void on_mem(u64, u64);
static void on_iomem(u64, u64);
static void deal_reserve(u64 addr, u64 size, bool ismem)
{
    extern struct arch_cpu the_cpu;
    the_cpu.rsp=read_rsp();
    if (addr > max_reserve_addr) {
        if (ismem)
            on_mem(addr, size);
        else
            on_iomem(addr, size);
        return;
    }
    for (int i = 0; i < reserved_nr; i++) {
        ulong rev_start = revmem[i].addr;
        ulong rev_size = revmem[i].size;

        if (addr >= rev_start + rev_size)
            continue;
        if (addr + size <= rev_start)
            continue;
        //total in reserve,just return.
        if (addr >= rev_start && addr + size <= rev_start + rev_size)
            return;
        else if (addr >= rev_start) {
            ulong newaddr, newsize;

            newaddr = rev_start + rev_size;
            newsize = size + addr - (rev_start + rev_size);
            //chunk in reserve
            deal_reserve(newaddr, newsize, ismem);
            return;
        }
        else {
            ASSERT(addr <= rev_start);
            //reserve total in mem zone
            ulong newaddr, newsize;

            newsize = rev_start - addr;
            deal_reserve(addr, newsize, ismem);
            newaddr = rev_start + size;
            newsize = size + addr - rev_start;
            deal_reserve(newaddr, newsize, ismem);
            return;
        }

    }
    if (ismem)
        on_mem(addr, size);
    else
        on_iomem(addr, size);
    return;
}

static void on_mem(u64 addr, u64 size)
{
    void free_phy_pages(ulong, ulong size);
    ulong low4k;

    DEBUG_PRINT("on_mem:%lx,%lx\n", addr, size);
    low4k = addr & 0xfff;
    if (low4k) {
        ulong low4ksize = 0x1000 - low4k;

        if (low4ksize >= size) {
            if (addr + size < __max_phy_mem_addr) {
                free_kheap_small(addr, size);
            }
        }
        size -= low4ksize;
        free_kheap_small(addr, low4ksize);
        addr += low4ksize;
        on_mem(addr, size);
        return;
    }
    size &= ~0xfff;

    if (!size)
        return;
    if (size < PAGE_SIZE) {
        if (addr + size < __max_phy_mem_addr) {
            free_kheap_4k(addr, size);
        }
    }
    else {
        ulong low = addr & (PAGE_SIZE - 1);
        ulong npg;

        if (low) {
            ulong lowsize = PAGE_SIZE - low;

            if (lowsize >= size) {
                if (addr + size < __max_phy_mem_addr) {
                    free_kheap_4k(addr, size);
                }
                return;
            }
            size -= lowsize;
            if (addr + lowsize < __max_phy_mem_addr) {
                free_kheap_4k(addr, lowsize);
            }
            addr += lowsize;
            on_mem(addr, size);
            return;
        }
        npg = size / PAGE_SIZE;
        free_phy_pages(addr, npg * PAGE_SIZE);
        size &= PAGE_SIZE - 1;
        addr += npg * PAGE_SIZE;
        if (size == 0)
            return;
        if (addr + size < __max_phy_mem_addr) {
            free_kheap_4k(addr, size);
        }
    }
}

ulong e820map_addr;
ulong e820map_size;
char *pe820;
void init_e820()
{
    memory_map_t *mmap;
    ulong minaddr = -1;
    ulong maxaddr = 0;
    ulong addr, msize;
    ulong phmemsize = 0;
    ulong maxmemaddr = 0;
    const ulong reserve_low_size = 0x10000;	//64k

    add_reserve(0, reserve_low_size);
    add_reserve(__kernel_start, __kernel_end - __kernel_start);
    add_map(IOAPIC_BASE/PAGE_SIZE+1);//map ioapic

    for (mmap = (memory_map_t *) pe820;
         (unsigned long)mmap < (ulong) pe820 + e820map_size;
         mmap = (memory_map_t *) ((unsigned long)mmap
                                  + mmap->size + sizeof(mmap->size))) {
        addr = (ulong) mmap->base_addr_high;
        addr <<= 32;
        addr += mmap->base_addr_low;
        msize = (ulong) mmap->length_high;
        msize <<= 32;
        msize += mmap->length_low;
        if (addr < minaddr)
            minaddr = addr;
        if (mmap->type == 1) {
            phmemsize += msize;
            if (addr > maxmemaddr)
                maxmemaddr = addr;
        }
        if (addr > minaddr && addr + msize < maxaddr) {
            printk("adjust addr %lx to %lx \n", addr, maxaddr);
            addr = maxaddr;
        }
        printf(" size = 0x%x, base_addr = %lx,"
               " length = %lx, type = 0x%x\n",
               (unsigned)mmap->size, addr, msize, (unsigned)mmap->type);
        if (mmap->type == 1)
            deal_reserve(addr, msize, true);
        else
            deal_reserve(addr, msize, false);

    }
}

void init_page_map(void)
{
    memory_map_t *mmap;
    ulong addr, msize;
    ulong lastend = 0;
    const ulong reserve_low_size = 0x10000;	//64k

    add_reserve(0, reserve_low_size);
    add_reserve(__kernel_start, __kernel_end - __kernel_start);

    for (mmap = (memory_map_t *) pe820;
         (unsigned long)mmap < (ulong) pe820 + e820map_size;
         mmap = (memory_map_t *) ((unsigned long)mmap
                                  + mmap->size + sizeof(mmap->size))) {
        addr = (ulong) mmap->base_addr_high;
        addr <<= 32;
        addr += mmap->base_addr_low;
        msize = (ulong) mmap->length_high;
        msize <<= 32;
        msize += mmap->length_low;
        if (addr > lastend) {
            ulong hsize = addr - lastend;
            ulong haddr;

            printk("hole:%lx length:%lx\n", lastend, addr - lastend);

            if (hsize < PAGE_SIZE)
                deal_reserve(lastend, addr - lastend, false);
            else {
                if (lastend & (PAGE_SIZE - 1)) {
                    deal_reserve(lastend,
                                 PAGE_SIZE - (lastend & (PAGE_SIZE - 1)),
                                 false);
                    lastend += PAGE_SIZE;
                    lastend &= ~(PAGE_SIZE - 1);
                }
                for (haddr = lastend; haddr + PAGE_SIZE < addr;
                     haddr += PAGE_SIZE) {
                    if (OK !=
                        map_page_p2v(haddr, haddr + IO_MEM_BASE,
                                     PTE_P | PTE_W | PTE_PWT | PTE_PCD |
                                     PTE_PS))
                        panic("No memory\n");
                }
            }
        }

        lastend = addr + msize;

    }

    printf("cr3:%lx,cr4:%lx\n", read_cr3(), read_cr4());
    printf("Start iomem remap\n");
    remap_iomem();

}

void init_bootload(u32 info_addr, u32 magic)
{
    int i;
    struct multiboot_info_type *mbi;
    ulong addr = (ulong) info_addr;
print_regs();
    mbi = (struct multiboot_info_type *)addr;
    if (magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        printk("bootload magic do not match %x,%x\n", magic,
               MULTIBOOT_BOOTLOADER_MAGIC);
        cli_hlt();
    }
    remaped = 0;
    e820map_addr = 0;
    e820map_size = 0;
    for (i = 0; i <= MAX_IO_REMAPS; i++)
        io_remap_pages[i] = -1;
    if (CHECK_FLAG(mbi->flags, 0))
        printf("mem_lower = %uKB, mem_upper = %uKB\n",
               (unsigned)mbi->mem_lower, (unsigned)mbi->mem_upper);

    /* boot_device 是否有效？ */
    if (CHECK_FLAG(mbi->flags, 1))
        printf("boot_device = 0x%x\n", (unsigned)mbi->boot_device);

    /* 是否有命令行参数？ */
    if (CHECK_FLAG(mbi->flags, 2))
        printf("cmdline = %s\n", (char *)(ulong) mbi->cmdline);

    /* mods_* 是否有效？ */
    if (CHECK_FLAG(mbi->flags, 3)) {
        module_t *mod;
        int i;

        printf("mods_count = %d, mods_addr = 0x%x\n",
               (int)mbi->mods_count, (int)mbi->mods_addr);
        for (i = 0, mod = (module_t *) (ulong) mbi->mods_addr;
             i < mbi->mods_count; i++, mod += sizeof(module_t))
            printf(" mod_start = 0x%x, mod_end = 0x%x, string = %s\n",
                   (unsigned)mod->mod_start,
                   (unsigned)mod->mod_end, (char *)(ulong) mod->string);
    }

    /* 第 4 位和第 5 位是互斥的！ */
    if (CHECK_FLAG(mbi->flags, 4) && CHECK_FLAG(mbi->flags, 5)) {
        printf("Both bits 4 and 5 are set.\n");
        return;
    }

    /* 是否有 ELF section header table？ */
    if (CHECK_FLAG(mbi->flags, 5)) {
        elf_section_header_table_t *elf_sec =
            (elf_section_header_table_t *) (&mbi->syms[0]);

        printf("elf_sec: num = %u, size = 0x%x,"
               " addr = 0x%x, shndx = 0x%x\n",
               (unsigned)elf_sec->num, (unsigned)elf_sec->size,
               (unsigned)elf_sec->addr, (unsigned)elf_sec->shndx);
    }

    /* mmap_* 是否有效？ */
    if (CHECK_FLAG(mbi->flags, 6)) {
        memory_map_t *mmap;
        ulong minaddr = -1;
        ulong maxaddr = 0;
        ulong maxmemaddr = 0;
        ulong addr, msize;
        ulong phmemsize = 0;

        printf("mmap_addr = 0x%x, mmap_length = 0x%x\n",
               (unsigned)mbi->mmap_addr, (unsigned)mbi->mmap_length);
        e820map_addr = (ulong) mbi->mmap_addr;
        e820map_size = (ulong) mbi->mmap_length;
        pe820 = kalloc(e820map_size);
        if (!pe820)
            panic("no memory");
        memcpy(pe820, (char *)e820map_addr, e820map_size);
        for (mmap = (memory_map_t *) e820map_addr;
             (unsigned long)mmap < (ulong) e820map_addr + e820map_size;
             mmap = (memory_map_t *) ((unsigned long)mmap
                                      + mmap->size + sizeof(mmap->size))) {
            addr = (ulong) mmap->base_addr_high;
            addr <<= 32;
            addr += mmap->base_addr_low;
            msize = (ulong) mmap->length_high;
            msize <<= 32;
            msize += mmap->length_low;
            if (addr < minaddr)
                minaddr = addr;
            if (addr + msize > maxaddr)
                maxaddr = addr + msize;
            if (mmap->type == 1) {
                phmemsize += msize;
                if (addr + msize > maxmemaddr)
                    maxmemaddr = addr + msize;
            }
            if (addr > minaddr && addr + msize < maxaddr) {
                printk("adjust addr %lx to %lx \n", addr, maxaddr);
                addr = maxaddr;
            }
            printf(" size = 0x%x, base_addr = %lx,"
                   " length = %lx, type = 0x%x\n",
                   (unsigned)mmap->size, addr, msize, (unsigned)mmap->type);

        }
        __max_phy_addr = maxaddr;
        __max_phy_mem_addr = maxmemaddr;
        printk("max_phy_addr:%lx,%lx\n", __max_phy_addr,read_rsp());
print_regs();
    }
    else {
        printk("no E820 mmap found\n");
        cli_hlt();
    }
}
