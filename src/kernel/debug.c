/*
 * E-comOS Microkernel - Early debug output implementation
 * Minimal VGA output for kernel boot messages only
 */

#include <stdint.h>
#include <kernel/debug.h>

#define VGA_MEMORY 0xB8000
#define VGA_WIDTH 80

static uint16_t* vga_buffer = (uint16_t*)VGA_MEMORY;
static size_t debug_column = 0;
static size_t debug_row = 0;

void early_debug_init(void) {
    // Clear first few lines for kernel messages
    for (size_t i = 0; i < VGA_WIDTH * 5; i++) {
        vga_buffer[i] = (uint16_t)' ' | 0x0F00;
    }
    debug_row = 0;
    debug_column = 0;
}

void early_debug_puts(const char* str) {
    while (*str) {
        if (*str == '\n') {
            debug_column = 0;
            debug_row++;
            if (debug_row >= 5) debug_row = 0; // Wrap after 5 lines
        } else {
            const size_t index = debug_row * VGA_WIDTH + debug_column;
            vga_buffer[index] = (uint16_t)*str | 0x0F00;
            debug_column++;
            if (debug_column >= VGA_WIDTH) {
                debug_column = 0;
                debug_row++;
                if (debug_row >= 5) debug_row = 0;
            }
        }
        str++;
    }
}