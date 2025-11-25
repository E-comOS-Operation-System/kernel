/*
 * E-comOS Microkernel - Boot definitions and structures
 */

#ifndef KERNEL_BOOT_H
#define KERNEL_BOOT_H

#include <stdint.h>

// Multiboot constants
#define MULTIBOOT_MAGIC 0x2BADB002
#define MULTIBOOT_FLAG_MEM     0x001
#define MULTIBOOT_FLAG_DEVICE  0x002
#define MULTIBOOT_FLAG_CMDLINE 0x004

// Multiboot information structure
struct multiboot_info {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;
    uint32_t mods_addr;
    uint32_t syms[4];
    uint32_t mmap_length;
    uint32_t mmap_addr;
} __attribute__((packed));

// Kernel main function declaration
void kernel_main(uint32_t multiboot_magic, uint32_t multiboot_info);

#endif