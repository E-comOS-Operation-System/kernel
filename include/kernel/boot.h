/*
    E-comOS Kernel - Boot Parameters
    Copyright (C) 2025,2026  Saladin5101

    Precondition:  filled by the UEFI bootloader before jumping to _start.
    Postcondition: read-only after kernelMain receives it.
*/

#ifndef KERNEL_BOOT_H
#define KERNEL_BOOT_H

#include <stdint.h>
#include <kernel/internal/types.h>
/* UEFI memory types (UEFI spec 2.x Table 7-6) */
#define EFI_RESERVED_MEMORY_TYPE   0
#define EFI_LOADER_CODE            1
#define EFI_LOADER_DATA            2
#define EFI_BOOT_SERVICES_CODE     3
#define EFI_BOOT_SERVICES_DATA     4
#define EFI_RUNTIME_SERVICES_CODE  5
#define EFI_RUNTIME_SERVICES_DATA  6
#define EFI_CONVENTIONAL_MEMORY    7
#define EFI_UNUSABLE_MEMORY        8
#define EFI_ACPI_RECLAIM_MEMORY    9
#define EFI_ACPI_MEMORY_NVS        10
#define EFI_MEMORY_MAPPED_IO       11
#define EFI_MEMORY_MAPPED_IO_PORT  12
#define EFI_PAL_CODE               13

/*
 * EFI_MEMORY_DESCRIPTOR — layout matches UEFI spec.
 * descSize from GetMemoryMap() may be larger than sizeof this struct;
 * always use bootParams->memoryDescriptorSize to stride the array.
 */
typedef struct {
    uint32_t type;
    uint32_t _pad;           /* UEFI spec: 4-byte pad before physicalStart */
    uint64_t physicalStart;
    uint64_t virtualStart;
    uint64_t numberOfPages;  /* 4 KB EFI pages */
    uint64_t attribute;
} __attribute__((packed)) EfiMemoryDescriptor;

/* Passed from bootloader to kernelMain via rdi (System V AMD64 ABI) */
typedef struct {
    u64 signature;                       // Magic number for validation
    u32 version;                         // Structure version
    u32 size;                            // Size of this structure

    u64 memoryMap;
    u64 memoryMapSize;
    u64 memoryMapKey;
    u64 memoryMapDescriptorSize;
    u64 memoryMapDescriptorVersion;

    u64 frameBuffer;                  // Address of framebuffer
    u32 frameBufferWidth;
    u32 frameBufferHeight;
    u32 frameBufferPitch;
    u32 frameBufferBpp;

    void* acpiRsdt;
    void* smbiosTable;
    u32 daemonProcessID;
    u64 sharedHeaderPhys;
    u64 kernelBase;
    u64 kernelSize;
    u64 kernelEntry;

    char commandLine[256];             // Optional command line for kernel

    void* runtimeService;
    u64 rtServicePhys;
    u64 sharedBuffer;
    uint64_t sharedBufferSize;
}BootParams;
/*
 * Linker-provided symbols marking the kernel image boundaries.
 * Used by mmInit to precisely reserve kernel pages.
 * Declared as arrays so taking their address gives the symbol value
 * without an extra indirection.
 */
extern uint8_t _kernelStart[];
extern uint8_t _kernelEnd[];

#endif
