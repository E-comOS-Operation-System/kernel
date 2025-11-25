/*
 * E-comOS Microkernel - Early initialization header
 */

#ifndef KERNEL_EARLY_INIT_H
#define KERNEL_EARLY_INIT_H

#include <stdint.h>

// Boot stage enumeration
typedef enum {
    BOOT_STAGE_START = 0,
    BOOT_STAGE_CPU_CHECK,
    BOOT_STAGE_MEMORY_MAP,
    BOOT_STAGE_READY
} boot_stage_t;

// CPU feature bits
#define CPU_FEATURE_FPU    (1 << 0)
#define CPU_FEATURE_PSE    (1 << 3)
#define CPU_FEATURE_TSC    (1 << 4)
#define CPU_FEATURE_MSR    (1 << 5)
#define CPU_FEATURE_PAE    (1 << 6)
#define CPU_FEATURE_APIC   (1 << 9)

// Function declarations
int early_kernel_init(uint32_t multiboot_magic, uint32_t multiboot_info);
boot_stage_t get_boot_stage(void);
uint32_t get_cpu_features(void);

#endif