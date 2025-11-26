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

#include <kernel/debug.h>
#include <stdint.h>

#define VGA_MEMORY 0xB8000
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

static uint16_t* vga_buffer = (uint16_t*)VGA_MEMORY;
static int cursor_x = 0;
static int cursor_y = 0;

void early_debug_init(void) {
    // Clear screen
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga_buffer[i] = 0x0720; // Space with white on black
    }
    cursor_x = cursor_y = 0;
}

void early_debug_puts(const char *str) {
    while (*str) {
        if (*str == '\n') {
            cursor_x = 0;
            cursor_y++;
        } else {
            int pos = cursor_y * VGA_WIDTH + cursor_x;
            vga_buffer[pos] = 0x0700 | *str; // White on black
            cursor_x++;
        }
        
        if (cursor_x >= VGA_WIDTH) {
            cursor_x = 0;
            cursor_y++;
        }
        
        if (cursor_y >= VGA_HEIGHT) {
            cursor_y = 0; // Simple wrap
        }
        
        str++;
    }
}