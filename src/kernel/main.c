/*
 * E-comOS Microkernel - Main entry point
 * Kernel initialization after receiving control from bootloader
 */

#include <stdint.h>
#include <kernel/boot.h>
#include <kernel/early_init.h>
#include <kernel/debug.h>

// Kernel main function - called from boot.s
void kernel_main(uint32_t multiboot_magic, uint32_t multiboot_info) {
    // Initialize early debug output
    early_debug_init();
    
    // Display minimal boot information
    early_debug_puts("E-comOS Microkernel v0.1\n");
    early_debug_puts("Boot complete\n");
    
    // Early kernel initialization
    int init_result = early_kernel_init(multiboot_magic, multiboot_info);
    if (init_result < 0) {
        early_debug_puts("FATAL: Init failed\n");
        return;
    }
    
    early_debug_puts("Microkernel ready\n");
    
    // TODO: Start userspace services (VGA driver, etc.)
    
    // Kernel main loop
    while (1) {
        // Wait for interrupts or events
        __asm__ volatile ("hlt");
    }
}