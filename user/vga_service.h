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
// user/vga_service.h - User-space VGA service definitions
// All functions run in user mode (no kernel privileges)
#ifndef VGA_SERVICE_H
#define VGA_SERVICE_H

#include <stdint.h>

// VGA text mode constants (user-space accesses physical address directly)
#define VGA_PHYS_BUFFER 0xB8000  // Physical address of VGA text buffer
#define VGA_WIDTH 80             // Number of columns
#define VGA_HEIGHT 25            // Number of rows

// VGA color attributes (low 4 bits: foreground; high 4 bits: background)
#define VGA_COLOR_BLACK 0x0
#define VGA_COLOR_BLUE 0x1
#define VGA_COLOR_GREEN 0x2
#define VGA_COLOR_CYAN 0x3
#define VGA_COLOR_RED 0x4
#define VGA_COLOR_MAGENTA 0x5
#define VGA_COLOR_BROWN 0x6
#define VGA_COLOR_LIGHT_GREY 0x7
#define VGA_COLOR_DARK_GREY 0x8
#define VGA_COLOR_LIGHT_BLUE 0x9
#define VGA_COLOR_LIGHT_GREEN 0xA  // Used in service status
#define VGA_COLOR_LIGHT_CYAN 0xB
#define VGA_COLOR_LIGHT_RED 0xC
#define VGA_COLOR_LIGHT_MAGENTA 0xD
#define VGA_COLOR_LIGHT_BROWN 0xE
#define VGA_COLOR_WHITE 0xF

// Combine foreground and background color
static inline uint8_t vga_make_color(uint8_t fg, uint8_t bg) {
    return fg | (bg << 4);
}

// Initialize VGA service (called once at startup)
void vga_service_init();

// Put a character at (x, y) with specified color
void vga_service_putchar(int x, int y, char c, uint8_t color);

// Clear screen with specified color (fill with spaces)
void vga_service_clear(uint8_t color);

// Entry point for VGA service (called by kernel scheduler)
void vga_service_main();

#endif