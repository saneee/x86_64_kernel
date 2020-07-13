#include <asm/cpu.h>
#include <yaos/printk.h>
#include <yaos/init.h>
static unsigned long cpu_khz_from_cpuid(void)
{
        unsigned int eax_base_mhz, ebx_max_mhz, ecx_bus_mhz, edx;

        if (boot_cpu_data.x86_vendor != X86_VENDOR_INTEL)
                return 0;

        if (boot_cpu_data.cpuid_level < 0x16)
                return 0;

        eax_base_mhz = ebx_max_mhz = ecx_bus_mhz = edx = 0;

        cpuid(0x16, &eax_base_mhz, &ebx_max_mhz, &ecx_bus_mhz, &edx);

        return eax_base_mhz * 1000;
}
static __init int init_tsc_call(bool isbp)
{
    printk("boot_cpu_data.x86_vendor:%d,boot_cpu_data.cpuid_level:%d，khz:%ld\n",boot_cpu_data.x86_vendor,boot_cpu_data.cpuid_level,cpu_khz_from_cpuid());
    return 0;
}

early_initcall(init_tsc_call);

