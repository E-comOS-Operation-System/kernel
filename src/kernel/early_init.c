/*
    E-comOS Kernel - A Microkernel for E-comOS
    Copyright (C) 2025  Saladin5101

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as published
    by the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <stdint.h>
#include <kernel/boot.h>

// CPU feature flags
static uint32_t cpu_features = 0;

// Early boot status
typedef enum {
    BOOT_STAGE_START = 0,
    BOOT_STAGE_CPU_CHECK,
    BOOT_STAGE_MEMORY_MAP,
    BOOT_STAGE_READY
} boot_stage_t;

static boot_stage_t boot_stage = BOOT_STAGE_START;

// Check CPU capabilities
static int check_cpu_features(void) {
    uint32_t eax, ebx, ecx, edx;
    
    // Check CPUID availability
    __asm__ volatile (
        "pushfl\n\t"
        "popl %%eax\n\t"
        "movl %%eax, %%ecx\n\t"
        "xorl $0x200000, %%eax\n\t"
        "pushl %%eax\n\t"
        "popfl\n\t"
        "pushfl\n\t"
        "popl %%eax\n\t"
        "xorl %%ecx, %%eax"
        : "=a" (eax) : : "ecx"
    );
    
    if (!(eax & 0x200000)) {
        return -1; // CPUID not supported
    }
    
    // Get CPU features
    __asm__ volatile (
        "cpuid"
        : "=a" (eax), "=b" (ebx), "=c" (ecx), "=d" (edx)
        : "a" (1)
    );
    
    cpu_features = edx;
    boot_stage = BOOT_STAGE_CPU_CHECK;
    return 0;
}

// Parse multiboot memory map
static void parse_memory_map(struct multiboot_info *mbi) {
    if (!(mbi->flags & MULTIBOOT_FLAG_MEM)) {
        return;
    }
    
    // Basic memory info available
    uint32_t mem_lower = mbi->mem_lower;
    uint32_t mem_upper = mbi->mem_upper;
    
    // Total memory in KB
    uint32_t total_mem = mem_lower + mem_upper;
    (void)total_mem; // Suppress unused variable warning
    
    boot_stage = BOOT_STAGE_MEMORY_MAP;
}

// Early kernel initialization
int early_kernel_init(uint32_t multiboot_magic, uint32_t multiboot_info) {
    struct multiboot_info *mbi = (struct multiboot_info *)multiboot_info;
    
    // Validate multiboot
    if (multiboot_magic != MULTIBOOT_MAGIC) {
        return -1;
    }
    
    // Check CPU capabilities
    if (check_cpu_features() < 0) {
        return -2;
    }
    
    // Parse memory information
    parse_memory_map(mbi);
    
    boot_stage = BOOT_STAGE_READY;
    return 0;
}

// Get current boot stage
boot_stage_t get_boot_stage(void) {
    return boot_stage;
}

// Get CPU features
uint32_t get_cpu_features(void) {
    return cpu_features;
}