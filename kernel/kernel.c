/*  E-comOS - A Microkernel-based Operating System
*   Copyright (C) 2025  Saladin5101
*
*   This program is free software: you can redistribute it and/or modify
*   it under the terms of the GNU Affero General Public License as published
*   by the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU Affero General Public License for more details.
*
*   You should have received a copy of the GNU Affero General Public License
*   along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
// kernel/kernel.c - E-comOS microkernel entry point
// Orchestrates initialization and starts first user-space service
#include "kernel.h"
#include <stdint.h>

// VGA color definition (black background, white text)
#define VGA_COLOR 0x07

// -------------------------- VGA Helper Functions --------------------------
// Write a character to VGA text buffer (kernel-space: uses kernel virtual address)
// Parameters:
//   x: Column (0 ~ VGA_WIDTH-1)
//   y: Row (0 ~ VGA_HEIGHT-1)
//   c: ASCII character to write
//   color: VGA color attribute (high 4 bits = bg, low 4 bits = fg)
void vga_put_char(int x, int y, char c, uint8_t color) {
    if (x >= VGA_WIDTH || y >= VGA_HEIGHT) {
        return; // Out of bounds
    }
    // Kernel accesses VGA buffer via virtual address (KERNEL_VIRT_BASE offset)
    uint64_t idx = y * VGA_WIDTH + x;
    void* vga_virt = (void*)(VGA_BUFFER + KERNEL_VIRT_BASE);
    ((uint8_t*)vga_virt)[idx * 2] = c;       // ASCII character
    ((uint8_t*)vga_virt)[idx * 2 + 1] = color; // Color attribute
}

// Clear VGA text buffer (fill with spaces)
// Parameter: color - VGA color attribute for cleared screen
void vga_clear_screen(uint8_t color) {
    for (int y = 0; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            vga_put_char(x, y, ' ', color);
        }
    }
}

// -------------------------- User-Space VGA Service --------------------------
// First user-space service: Writes "VGA Service Running" to screen
// Runs in user-space (low virtual address, no kernel privileges)
void user_vga_service() {
    // User-space accesses VGA buffer via physical address (mapped to user space)
    uint64_t idx = 0 * VGA_WIDTH + 15; // Row 0, Column 15
    void* vga_phys = (void*)VGA_BUFFER;
    
    // Write "VGA Service Running"
    char* msg = "VGA Service Running";
    for (int i = 0; msg[i] != '\0'; i++) {
        ((uint8_t*)vga_phys)[(idx + i) * 2] = msg[i];
        ((uint8_t*)vga_phys)[(idx + i) * 2 + 1] = 0x0A; // Green text (0x0A = black bg, green fg)
    }
    
    // Loop forever (user-space service should not exit)
    while (1);
}

// -------------------------- Kernel Entry Point --------------------------
// Called by DOS25 bootloader (64-bit long mode, physical address 0x100000)
void kernel_main() {
    // 1. Initialize VGA text mode (clear screen + show kernel startup msg)
    vga_clear_screen(VGA_COLOR);
    char* kernel_msg = "E-comOS Kernel Started";
    for (int i = 0; kernel_msg[i] != '\0'; i++) {
        vga_put_char(i, 0, kernel_msg[i], VGA_COLOR);
    }
    
    // 2. Initialize memory management (paging for address isolation)
    mm_init();
    
    // 3. Initialize task scheduler (for user-space tasks)
    sched_init();
    
    // 4. Create first user-space task: VGA service
    uint64_t vga_pid = sched_create_task(user_vga_service, 1); // 1 = user-space
    if (vga_pid == 0) {
        vga_put_char(0, 5, 'V', 0x04); // Red 'V' for VGA service create fail
        vga_put_char(1, 5, 'G', 0x04);
        vga_put_char(2, 5, 'A', 0x04);
        vga_put_char(3, 5, ' ', 0x04);
        vga_put_char(4, 5, 'F', 0x04);
        vga_put_char(5, 5, 'a', 0x04);
        vga_put_char(6, 5, 'i', 0x04);
        vga_put_char(7, 5, 'l', 0x04);
        while (1);
    }
    
    // 5. Start scheduling (switch to user-space VGA service)
    sched_switch_task();
    
    // Should never reach here (sched_switch_task() doesn't return)
    while (1);
}