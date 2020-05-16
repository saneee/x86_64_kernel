/* Simple linker script for the JOS kernel.
   See the GNU ld 'info' manual ("info ld") to learn the syntax. */

/* OUTPUT_FORMAT("elf32-i386", "elf32-i386", "elf32-i386") */
OUTPUT_FORMAT("elf64-x86-64", "elf64-x86-64", "elf64-x86-64")
OUTPUT_ARCH(i386:x86-64)
ENTRY(phys_startup_64)

mboot_load_addr = 0x00100000;

SECTIONS
{

	/* Link the kernel at this address: "." means the current address */
        /* Must be equal to KERNLINK */
	. = 0xFFFFFFFF80100000; 

	PROVIDE(begin = .);
	phys_startup_64 = _start - 0xffffffff80000000;
	.text : AT(mboot_load_addr) {
		*(.text .rela.text .stub .text.* .gnu.linkonce.t.*)
	}

	PROVIDE(etext = .);	/* Define the 'etext' symbol to this value */

	.rodata : AT(ADDR(.rodata) - 0xffffffff80000000) {
		*(.rodata .rodata.* .gnu.linkonce.r.*)
	}

	/* Adjust the address for the data segment to the next page */
	. = ALIGN(0x1000);

	/* Conventionally, Unix linkers provide pseudo-symbols
	 * etext, edata, and end, at the end of the text, data, and bss.
	 * For the kernel mapping, we need the address at the beginning
	 * of the data section, but that's not one of the conventional
	 * symbols, because the convention started before there was a
	 * read-only rodata section between text and data. */
	PROVIDE(data = .);

	/* The data segment */
	.data : AT(ADDR(.data) - 0xffffffff80000000) {
		*(.data)
	}

	. = ALIGN(0x1000);

	PROVIDE(edata = .);

	.bss : AT(ADDR(.bss) - 0xffffffff80000000) {
		*(.bss)
		*(COMMON)
	}

	. = ALIGN(0x1000);

	PROVIDE(end = .);
	__per_cpu_load = .;
        .percpu  0 :AT(__per_cpu_load - 0xffffffff80000000){
                _percpu_start = .;
                *(.percpu.first)
                *(.percpu)
                *(.percpu.last) 
                . = ALIGN(4096);
                 _percpu_end = .;
        } 
	 

	/DISCARD/ : {
		*(.eh_frame .rela.eh_frame .note.GNU-stack)
	}
}
mboot_load_end = mboot_load_addr + (edata - begin);
mboot_bss_end = mboot_load_addr + (end - begin);
mboot_entry_addr = mboot_load_addr + (mboot_entry - begin);
_percpu_size = _percpu_end - _percpu_start;
init_percpu_start = __per_cpu_load;
init_percpu_end = __per_cpu_load + _percpu_size;
_kernel_end = end + _percpu_size;
_kernel_start = begin;
init_desc = the_desc + __per_cpu_load;
the_cpu = __the_cpu + __per_cpu_load;
init_per_cpu__current_thread = current_thread + __per_cpu_load;

