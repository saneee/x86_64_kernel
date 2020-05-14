/* zhuhanjun 2015/10/29 
per cpu struct
*/
#ifndef ARCH_X86_64_CPU_H
#define ARCH_X86_64_CPU_H 1
#include <yaos/types.h>
#define INTR_GATE_TYPE 14
static const ulong cr0_pe = 1u << 0;
static const ulong cr0_mp = 1u << 1;
static const ulong cr0_em = 1u << 2;
static const ulong cr0_ts = 1u << 3;
static const ulong cr0_et = 1u << 4;
static const ulong cr0_ne = 1u << 5;
static const ulong cr0_wp = 1u << 16;
static const ulong cr0_am = 1u << 18;
static const ulong cr0_nw = 1u << 29;
static const ulong cr0_cd = 1u << 30;
static const ulong cr0_pg = 1u << 31;

static const ulong cr4_vme = 1u << 0;
static const ulong cr4_pvi = 1u << 1;
static const ulong cr4_tsd = 1u << 2;
static const ulong cr4_de = 1u << 3;
static const ulong cr4_pse = 1u << 4;
static const ulong cr4_pae = 1u << 5;
static const ulong cr4_mce = 1u << 6;
static const ulong cr4_pge = 1u << 7;
static const ulong cr4_pce = 1u << 8;
static const ulong cr4_osfxsr = 1u << 9;
static const ulong cr4_osxmmexcpt = 1u << 10;
static const ulong cr4_vmxe = 1u << 13;
static const ulong cr4_smxe = 1u << 14;
static const ulong cr4_fsgsbase = 1u << 16;
static const ulong cr4_pcide = 1u << 17;
static const ulong cr4_osxsave = 1u << 18;
static const ulong cr4_smep = 1u << 20;

static const ulong rflags_if = 1u << 9;

static const u32 xcr0 = 0;
static const u64 xcr0_x87 = 1u << 0;
static const u64 xcr0_sse = 1u << 1;
static const u64 xcr0_avx = 1u << 2;


struct cpu_features_type {
    bool sse3;
    bool ssse3;
    bool cmpxchg16b;
    bool sse4_1;
    bool sse4_2;
    bool x2apic;
    bool tsc_deadline;
    bool xsave;
    bool osxsave;
    bool avx;
    bool rdrand;
    bool clflush;
    bool fsgsbase;
    bool repmovsb;
    bool gbpage;
    bool invariant_tsc;
    bool kvm_clocksource;
    bool kvm_clocksource2;
    bool kvm_clocksource_stable;
    bool kvm_pv_eoi;
    bool xen_clocksource;
    bool xen_vector_callback;
    bool xen_pci;
};

struct cpuid_result {
    u32 a, b, c, d;
};
static inline void native_cpuid(unsigned int *eax, unsigned int *ebx,
                                unsigned int *ecx, unsigned int *edx)
{
        /* ecx is often an input as well as an output. */
        asm volatile("cpuid"
            : "=a" (*eax),
              "=b" (*ebx),
              "=c" (*ecx),
              "=d" (*edx)
            : "0" (*eax), "2" (*ecx)
            : "memory");
}
extern ulong read_rip();
static inline ulong read_rsp() {
    ulong r;
    asm volatile ("mov %%rsp, %0" : "=r"(r));
    return r;
}

static inline ulong read_rbp() {
    ulong r;
    asm volatile ("mov %%rbp, %0" : "=r"(r));
    return r;
}
static inline ulong read_cr0() {
    ulong r;
    asm volatile ("mov %%cr0, %0" : "=r"(r));
    return r;
}

static inline void write_cr0(ulong r) {
    asm volatile ("mov %0, %%cr0" : : "r"(r));
}

static inline ulong read_cr2() {
    ulong r;
    asm volatile ("mov %%cr2, %0" : "=r"(r));
    return r;
}
static inline void write_cr2(ulong r) {
    asm volatile ("mov %0, %%cr2" : : "r"(r));
}

static inline ulong read_cr3() {
    ulong r;
    asm volatile ("mov %%cr3, %0" : "=r"(r));
    return r;
}

static inline void write_cr3(ulong r) {
    asm volatile ("mov %0, %%cr3" : : "r"(r));
}
static inline ulong read_cr4() {
    ulong r;
    asm volatile ("mov %%cr4, %0" : "=r"(r));
    return r;
}

static inline void write_cr4(ulong r) {
    asm volatile ("mov %0, %%cr4" : : "r"(r));
}
static inline ulong read_cr8() {
    ulong r;
    asm volatile ("mov %%cr8, %0" : "=r"(r));
    return r;
}

static inline void write_cr8(ulong r) {
    asm volatile ("mov %0, %%cr8" : : "r"(r));
}

struct desc_ptr {
    u16 limit;
    ulong addr;
} __attribute__((packed));

static inline void lgdt(struct desc_ptr * ptr) {
    asm volatile ("lgdt %0" : : "m"(*ptr));
}

static inline void sgdt(struct desc_ptr * ptr) {
    asm volatile ("sgdt %0" : "=m"(*ptr));
}

static inline void lidt(struct desc_ptr * ptr) {
    asm volatile ("lidt %0" : : "m"(*ptr));
}


static inline void sidt(struct desc_ptr * ptr) {
    asm volatile ("sidt %0" : "=m"(*ptr));
}

static inline void ltr(u16 tr) {
    asm volatile("ltr %0" : : "rm"(tr));
}

static inline u16 read_tr() {
    u16 tr;
    asm volatile("str %0" : "=rm"(tr));
    return tr;
}
static inline u16 read_cs() {
    u16 r;
    asm volatile ("mov %%cs, %0" : "=rm"(r));
    return r;
}

static inline u16 read_ds() {
    u16 r;
    asm volatile ("mov %%ds, %0" : "=rm"(r));
    return r;
}

static inline void write_ds(u16 r) {
    asm volatile ("mov %0, %%ds" : : "rm"(r));
}

static inline u16 read_es() {
    u16 r;
    asm volatile ("mov %%es, %0" : "=rm"(r));
    return r;
}

static inline void write_es(u16 r) {
    asm volatile ("mov %0, %%es" : : "rm"(r));
}

static inline u16 read_fs() {
    u16 r;
    asm volatile ("mov %%fs, %0" : "=rm"(r));
    return r;
}
static inline void write_fs(u16 r) {
    asm volatile ("mov %0, %%fs" : : "rm"(r));
}

static inline u16 read_gs() {
    u16 r;
    asm volatile ("mov %%gs, %0" : "=rm"(r));
    return r;
}

static inline void write_gs(u16 r) {
    asm volatile ("mov %0, %%gs" : : "rm"(r));
}

static inline u16 read_ss() {
    u16 r;
    asm volatile ("mov %%ss, %0" : "=rm"(r));
    return r;
}

static inline void write_ss(u16 r) {
    asm volatile ("mov %0, %%ss" : : "rm"(r));
}
static inline u64 rdmsr(u32 index) {
    u32 lo, hi;
    asm volatile ("rdmsr" : "=a"(lo), "=d"(hi) : "c"(index));
    return lo | ((u64)hi << 32);
}

static inline void wrmsr(u32 index, u64 data) {
    u32 lo = data, hi = data >> 32;
    asm volatile ("wrmsr" : : "c"(index), "a"(lo), "d"(hi));
}

static inline bool wrmsr_safe(u32 index, u64 data) {
    u32 lo = data, hi = data >> 32;

    bool ret = true;
    asm volatile ("1: \n\t"
                  "wrmsr\n\t"
                  "2: \n\t"
                  ".pushsection .text.fixup, \"ax\" \n\t"
                  "3: \n\t"
                  "xor %[ret], %[ret]\n\t"
                  "jmp 2b \n\t"
                  ".popsection \n\t"
                  ".pushsection .fixup, \"aw\" \n\t"
                  ".quad 1b, 3b \n\t"
                  ".popsection\n"
            :  [ret]"+r"(ret)
            : "c"(index), "a"(lo), "d"(hi));

    return ret;
}

static inline void wrfsbase(u64 data)
{
    asm volatile("wrfsbase %0" : : "r"(data));
}

static inline void cli_hlt() {
    asm volatile ("cli; hlt" : : : "memory");
}

static inline void sti_hlt() {
    asm volatile ("sti; hlt" : : : "memory");
}
static inline u8 inb(u16 port)
{
    u8 r;
    asm volatile ("inb %1, %0":"=a" (r):"dN" (port));
    return r;
}

static inline u16 inw(u16 port)
{
    u16 r;
    asm volatile ("inw %1, %0":"=a" (r):"dN" (port));
    return r;
}

static inline u32 inl(u16 port)
{
    u32 r;
    asm volatile ("inl %1, %0":"=a" (r):"dN" (port));
    return r;
}
static inline void insl(void *addr, int cnt, u16 port)
{
    asm volatile ("rep insl"
        :"+D" (addr), "+c" (cnt)
        : "d" (port)
        : "memory", "cc");
}

static inline void outb(u8 val, u16 port)
{
    asm volatile ("outb %0, %1"::"a" (val), "dN" (port));

}

static inline void outw(u16 val, u16 port)
{
    asm volatile ("outw %0, %1"::"a" (val), "dN" (port));

}
static inline void outl(u32 val, u16 port)
{
    asm volatile ("outl %0, %1"::"a" (val), "dN" (port));

}

/* cnt is not bytes, 4byte counter */
static inline void outsl(void *addr, int cnt, u16 port)
{
    asm volatile ("rep outsl"
        :"+S" (addr), "+c" (cnt)
        : "d" (port)
        : "cc");
}
static inline void sti()
{
    asm volatile ("sti" : : : "memory");
}

static inline void cli()
{
    asm volatile ("cli" : : : "memory");
}

__attribute__((no_instrument_function))
static inline void cli_notrace();

static inline void cli_notrace()
{
    asm volatile ("cli" : : : "memory");
}

static inline u64 rdtsc()
{

    unsigned low,high;
    asm volatile("rdtsc" : "=a" (low), "=d" (high));


    return  ((low) | ((u64)(high) << 32));
}

static inline u64 ticks()
{
    return rdtsc();
}
struct fpu_state {
    char legacy[512];
    char xsavehdr[24];
    char reserved[40];
    char ymm[256];
} __attribute__((packed));

static inline void fxsave(struct fpu_state* s)
{
    asm volatile("fxsaveq %0" : "=m"(*s));
}

static inline void fxrstor(struct fpu_state* s)
{
    asm volatile("fxrstorq %0" : : "m"(*s));
}

static inline void xsave(struct fpu_state* s, u64 mask)
{
    u32 a = mask, d = mask >> 32;
    asm volatile("xsaveq %[fpu]" : [fpu]"=m"(*s) : "a"(a), "d"(d));
}

static inline void xsaveopt(struct fpu_state* s, u64 mask)
{
    u32 a = mask, d = mask >> 32;
    asm volatile("xsaveoptq %[fpu]" : [fpu]"=m"(*s) : "a"(a), "d"(d));
}

static inline void xrstor(const struct fpu_state* s, u64 mask)
{
    u32 a = mask, d = mask >> 32;
    asm volatile("xrstorq %[fpu]" : : [fpu]"m"(*s), "a"(a), "d"(d));
}

static inline void write_xcr(u32 reg, u64 val)
{
    u32 a = val;
    u32 d = val >> 32;
    asm volatile("xsetbv" : : "c"(reg), "a"(a), "d"(d));
}

static inline u64 read_xcr(u32 reg)
{
    u32 a, d;
    asm volatile("xgetbv" : "=a"(a), "=d"(d) : "c"(reg));
    return (a | ((u64)d << 32));
}
static inline void init_fpu()
{
    asm volatile ("fninit" ::: "memory");
    unsigned int csr = 0x1f80;
    asm volatile ("ldmxcsr %0" : : "m" (csr));
}

static inline void lfence()
{
    asm volatile("lfence");
}
static inline bool rdrand(u64* dest)
{
    unsigned char ok;
    asm volatile ("rdrand %0; setc %1"
        : "=r" (*dest), "=qm" (ok)
        :
        : "cc");
    return ok;
}
static inline uint32_t stmxcsr()
{
    uint32_t addr;
    asm volatile ("stmxcsr %0" : "=m" (addr));
    return addr;
}

static inline void ldmxcsr(uint32_t addr)
{
    asm volatile ("ldmxcsr %0" : : "m" (addr));
}

static inline uint16_t fnstcw()
{
    uint16_t addr;
    asm volatile ("fnstcw %0" : "=m" (addr));
    return addr;
}
static inline void fldcw(uint16_t addr)
{
    asm volatile ("fldcw %0" : : "m" (addr));
}

static inline unsigned int cpuid_apic_id()
{
    u32 eax = 1;
    u32 ecx = 0;
    u32 ebx, edx;

    native_cpuid(&eax, &ebx, &ecx, &edx);
    return ebx>>24;  
}


void print_regs();
#endif
