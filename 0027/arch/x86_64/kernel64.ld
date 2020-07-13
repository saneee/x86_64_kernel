/* Simple linker script for the JOS kernel.
   See the GNU ld 'info' manual ("info ld") to learn the syntax. */

/* OUTPUT_FORMAT("elf32-i386", "elf32-i386", "elf32-i386") */
OUTPUT_FORMAT("elf64-x86-64", "elf64-x86-64", "elf64-x86-64")
OUTPUT_ARCH(i386:x86-64)
ENTRY(phys_startup_64)

mboot_load_addr = 0x001000000;

SECTIONS
{

	/* Link the kernel at this address: "." means the current address */
        /* Must be equal to KERNLINK */
	. = (0xFFFFFFFF80000000 + ALIGN(0x1000000, 0x1000000)); 

	PROVIDE(begin = .);
	phys_startup_64 = _start - 0xffffffff80000000;
	.text : AT(mboot_load_addr) {
		*(.text .rela.text .stub .text.* .gnu.linkonce.t.*)
	}

	PROVIDE(etext = .);	/* Define the 'etext' symbol to this value */

	.rodata : AT(ADDR(.rodata) - 0xffffffff80000000) {
		*(.rodata .rodata.* .gnu.linkonce.r.*)
                . = ALIGN(0x10);
                _lua_module_data_start = .;
                *(.lua_module_data)
                _lua_module_data_end = .;
                . = ALIGN(0x20);
                _lua_file_data_start = .;
                *(.lua_file_data)
                _lua_file_data_end = .;  

	}
        
	/* Adjust the address for the data segment to the next page */
	. = ALIGN(0x1000);
        PROVIDE(erodata = .);

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
         .init.begin : {
        __init_begin = .;
        }
 . = ALIGN((1 << 12)); .init.text : AT(ADDR(.init.text) - 0xffffffff80000000) { _sinittext = .; *(.init.text) _einittext = .; }
.init.data : AT(ADDR(.init.data) - 0xffffffff80000000) {
*(.init.data)
 . = ALIGN(8); __start_mcount_loc = .; *(__mcount_loc) __stop_mcount_loc = .; *(.init.rodata)
. = ALIGN(8); __start_ftrace_events = .;
 *(_ftrace_events)
 __stop_ftrace_events = .; __start_ftrace_enum_maps = .;
*(_ftrace_enum_map) __stop_ftrace_enum_maps = .;
 . = ALIGN(8); __start_syscalls_metadata = .; *(__syscalls_metadata) __stop_syscalls_metadata = .;
. = ALIGN(8); __start_kprobe_blacklist = .; *(_kprobe_blacklist) __stop_kprobe_blacklist = .;
. = ALIGN(8); __clk_of_table = .; *(__clk_of_table) *(__clk_of_table_end) . = ALIGN(8); __cpu_method_of_table = .; *(__cpu_method_of_table) *(__cpu_method_of_table_end)
. = ALIGN(8); __cpuidle_method_of_table = .; *(__cpuidle_method_of_table) *(__cpuidle_method_of_table_end)
. = ALIGN(32); __dtb_start = .; *(.dtb.init.rodata) __dtb_end = .;
. = ALIGN(32); __earlycon_table = .; *(__earlycon_table) *(__earlycon_table_end)
. = ALIGN(8); __earlycon_of_table = .; *(__earlycon_of_table) *(__earlycon_of_table_end)
. = ALIGN(16); __setup_start = .; *(.init.setup) __setup_end = .;
__initcall_start = .; *(.initcallearly.init)
__initcall0_start = .; *(.initcall0.init) *(.initcall0s.init)
__initcall1_start = .; *(.initcall1.init) *(.initcall1s.init)
__initcall2_start = .; *(.initcall2.init) *(.initcall2s.init)
__initcall3_start = .; *(.initcall3.init) *(.initcall3s.init)
__initcall4_start = .; *(.initcall4.init) *(.initcall4s.init)
__initcall5_start = .; *(.initcall5.init) *(.initcall5s.init)
__initcallrootfs_start = .; *(.initcallrootfs.init) *(.initcallrootfss.init)
__initcall6_start = .; *(.initcall6.init) *(.initcall6s.init)
__initcall7_start = .; *(.initcall7.init) *(.initcall7s.init)
__initcall8_start = .; *(.initcall8.init) *(.initcall8s.init)

__initcall_end = .;
__con_initcall_start = .; *(.con_initcall.init) __con_initcall_end = .; __security_initcall_start = .; *(.security_initcall.init) __security_initcall_end = .;
. = ALIGN(4); __initramfs_start = .; *(.init.ramfs) . = ALIGN(8); *(.init.ramfs.info) }


        . = ALIGN(0x80);

        .module_data : {
                _module_data_start = .;
                *(.module_data)
                 _module_data_end = .;
        }

        . = ALIGN(0x80);

        .yaoscall_data : {
                _yaoscall_data_start = .;
                *(.yaoscall_data)
                 _yaoscall_data_end = .;
        }
 . = ALIGN(8);
 .altinstructions :  {
  _alt_instructions_start = .;
  *(.altinstructions)
  _alt_instructions_end = .;
 }
.altinstr_replacement : {
  *(.altinstr_replacement)
 }
 . = ALIGN((1 << 12));
        .init.end : AT(ADDR(.init.end)) {
           __init_end = .;
        }

	__per_cpu_load = .;
        .percpu  0 :AT(__per_cpu_load - 0xffffffff80000000){
                _percpu_start = .;
                *(.percpu.first) 
		. =  ALIGN((1 << 12));
		*(.percpu.page_aligned)
		. = ALIGN((1<<6));
		*(.percpu.read_mostly)
		. = ALIGN((1<<6));
                *(.percpu)
		*(.percpu.shared_aligned)
                *(.percpu.last) 
                . = ALIGN(4096);
                 _percpu_end = .;
        } 
	. = __per_cpu_load + SIZEOF(.percpu); 
        
        . = ALIGN((1 << 12));
        PROVIDE(edata = .);

        .bss : AT(ADDR(.bss) - 0xffffffff80000000) {
           __bss_start = .;
                *(.bss.page_aligned)
                *(.bss)
                *(COMMON)
           . = ALIGN((1 << 12));
           __bss_stop = .;
        }
	PROVIDE(end = .);

        . = ALIGN(0x1000);
        
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
init_per_cpu__the_cpu = the_cpu + __per_cpu_load;
init_per_cpu__current_thread = current_thread + __per_cpu_load;
__per_cpu_start = _percpu_start;
__per_cpu_size = _percpu_size;
__per_cpu_end = _percpu_end;
init_per_cpu__init_stack = init_stack + __per_cpu_load;
ro_start = begin;
ro_end = erodata;
