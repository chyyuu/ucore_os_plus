// FS/GS base registers
#define MSR_FS_BASE     0xc0000100
#define MSR_GS_BASE     0xc0000101
#define MSR_GS_KERNBASE 0xc0000102

// SYSCALL and SYSRET registers
#define MSR_STAR        0xc0000081
#define MSR_LSTAR       0xc0000082
#define MSR_CSTAR       0xc0000083
#define MSR_SFMASK      0xc0000084

#define MSR_INTEL_MISC_ENABLE 0x1a0
#define MISC_ENABLE_PEBS_UNAVAILABLE (1<<12) // Read-only

// AMD performance event-select registers
#define MSR_AMD_PERF_SEL0  0xC0010000
#define MSR_AMD_PERF_SEL1  0xC0010001
#define MSR_AMD_PERF_SEL2  0xC0010002
#define MSR_AMD_PERF_SEL3  0xC0010003
// AMD performance event-count registers
#define MSR_AMD_PERF_CNT0  0xC0010004
#define MSR_AMD_PERF_CNT1  0xC0010005
#define MSR_AMD_PERF_CNT2  0xC0010006
#define MSR_AMD_PERF_CNT3  0xC0010007

// Intel performance event-select registers
#define MSR_INTEL_PERF_SEL0 0x00000186
// Intel performance event-count registers
#define MSR_INTEL_PERF_CNT0 0x000000c1
#define MSR_INTEL_PERF_GLOBAL_STATUS   0x38e
#define PERF_GLOBAL_STATUS_PEBS        (1ull << 62)
#define MSR_INTEL_PERF_GLOBAL_CTRL     0x38f
#define MSR_INTEL_PERF_GLOBAL_OVF_CTRL 0x390

#define MSR_INTEL_PERF_CAPABILITIES 0x345 // RO
#define MSR_INTEL_PEBS_ENABLE       0x3f1
#define MSR_INTEL_PEBS_LD_LAT       0x3f6
#define MSR_INTEL_DS_AREA           0x600

// Intel prefetch control registers.
// See bits-767/boot/cfg/configure.nhm.common.cfg in
// http://biosbits.org/downloads/bits-767.zip
#define MSR_INTEL_PREFETCH  0x000001a4
// Set any of these to disable the corresponding prefetcher
#define MSR_INTEL_PREFETCH_DISABLE_MLC_STREAMER   0x01
#define MSR_INTEL_PREFETCH_DISABLE_MLC_SPATIAL    0x02
#define MSR_INTEL_PREFETCH_DISABLE_DCU_STREAMER   0x04
#define MSR_INTEL_PREFETCH_DISABLE_DCU_IP         0x08
// Only for CPUID mode 0xc family 0x6 type 0 extmodel 0x2 extfamily 0
// (CPUID_FEATURES eax 0x206c0, ignore stepping).
#define MSR_INTEL_PREFETCH_DISABLE_DATA_REUSE     0x40

// Common event-select bits
#define PERF_SEL_USR        (1ULL << 16)
#define PERF_SEL_OS         (1ULL << 17)
#define PERF_SEL_EDGE       (1ULL << 18)
#define PERF_SEL_INT        (1ULL << 20)
#define PERF_SEL_ENABLE     (1ULL << 22)
#define PERF_SEL_INV        (1ULL << 23)
#define PERF_SEL_CMASK_SHIFT 24

// APIC Base Address Register MSR
#define MSR_APIC_BAR        0x0000001b
#define APIC_BAR_XAPIC_EN   (1 << 11)
#define APIC_BAR_X2APIC_EN  (1 << 10)
