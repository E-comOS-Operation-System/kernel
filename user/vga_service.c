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
// user/vga_service.c - User-space VGA service implementation
// Runs in user mode, accesses VGA buffer directly (no kernel calls)
#include "vga_service.h"
#include <stddef.h>

// Pointer to VGA buffer (user-space uses physical address directly)
static uint16_t* vga_buffer;

// Initialize VGA service: set buffer pointer and clear screen
void vga_service_init() {
    // Map VGA physical buffer to user-space address (kernel already mapped it)
    vga_buffer = (uint16_t*)VGA_PHYS_BUFFER;
    // Clear screen with black background, dark grey text
    vga_service_clear(vga_make_color(VGA_COLOR_DARK_GREY, VGA_COLOR_BLACK));
}

// Put a character at (x, y) with color (user-space implementation)
void vga_service_putchar(int x, int y, char c, uint8_t color) {
    if (x < 0 || x >= VGA_WIDTH || y < 0 || y >= VGA_HEIGHT) {
        return; // Ignore out-of-bounds
    }
    const size_t index = y * VGA_WIDTH + x;
    vga_buffer[index] = (uint16_t)c | (uint16_t)color << 8;
}

// Clear screen (user-space implementation)
void vga_service_clear(uint8_t color) {
    for (int y = 0; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            const size_t index = y * VGA_WIDTH + x;
            vga_buffer[index] = (uint16_t)' ' | (uint16_t)color << 8;
        }
    }
}

// VGA service main loop: shows status and waits for IPC messages (future)
void vga_service_main() {
    vga_service_init();
    
    // Show service status (row 0, column 20) in light green
    const char* status = "VGA Service [User Mode] Running";
    uint8_t color = vga_make_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    for (int i = 0; status[i] != '\0'; i++) {
        vga_service_putchar(20 + i, 0, status[i], color);
    }
    
    // Future: Wait for IPC messages (e.g., from shell to print text)
    // For now: Loop forever to keep the service running
    while (1) {
        // Halt CPU to save power (user-space can use hlt with proper permissions)
        asm volatile ("hlt");
    }
}