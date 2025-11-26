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
#include <kernel/early_init.h>
#include <kernel/debug.h>
#include <kernel/arch/interrupts.h>

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
    
    // Initialize interrupt system
    early_debug_puts("Initializing interrupts...\n");
    idt_init();
    irq_remap();
    
    // Set up ISR handlers
    idt_set_gate(0, (uint32_t)isr0, 0x08, 0x8E);
    idt_set_gate(13, (uint32_t)isr13, 0x08, 0x8E);  // GPF
    idt_set_gate(14, (uint32_t)isr14, 0x08, 0x8E);  // Page fault
    
    // Set up IRQ handlers
    idt_set_gate(32, (uint32_t)irq0, 0x08, 0x8E);   // Timer
    idt_set_gate(33, (uint32_t)irq1, 0x08, 0x8E);   // Keyboard
    
    // Enable interrupts
    __asm__ volatile ("sti");
    early_debug_puts("Interrupts enabled\n");
    
    early_debug_puts("Microkernel ready\n");
    
    // TODO: Start userspace services (VGA driver, etc.)
    
    // Kernel main loop
    while (1) {
        // Wait for interrupts or events
        __asm__ volatile ("hlt");
    }
}