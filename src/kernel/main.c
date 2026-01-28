/*
    E-comOS Kernel - A Microkernel for E-comOS
    Copyright (C) 2025,2026  Saladin5101

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
#include <kernel/arch/x86_64.h>
#include <kernel/mm.h>
#include <kernel/sched.h>
#include <kernel/ipc.h>
#include <kernel/syscall.h>



// Simple VGA text mode buffer
// VGA memory starts at 0xB8000 in 80x25 text mode
#define VGA_MEMORY ((volatile uint16_t*)0xB8000)

// Current cursor position for text output
static int cursor_x = 0;
static int cursor_y = 0;

// Clear screen with specified background color
void clear_screen(uint8_t color) {
    uint16_t blank = 0x0720; // Space character with specified color
    for (int i = 0; i < 80 * 25; i++) {
        VGA_MEMORY[i] = blank;
    }
    cursor_x = cursor_y = 0;
}

// Print a single character to VGA display
void putchar(char c, uint8_t color) {
    if (c == '\n') {
        // Newline handling
        cursor_x = 0;
        cursor_y++;
    } else if (c == '\r') {
        // Carriage return
        cursor_x = 0;
    } else {
        // Normal character
        int pos = cursor_y * 80 + cursor_x;
        VGA_MEMORY[pos] = (color << 8) | c;
        cursor_x++;
    }
    
    // Handle line wrapping
    if (cursor_x >= 80) {
        cursor_x = 0;
        cursor_y++;
    }
    
    // Handle screen scrolling
    if (cursor_y >= 25) {
        // Scroll up one line
        for (int i = 0; i < 80 * 24; i++) {
            VGA_MEMORY[i] = VGA_MEMORY[i + 80];
        }
        // Clear bottom line
        for (int i = 80 * 24; i < 80 * 25; i++) {
            VGA_MEMORY[i] = 0x0F20;
        }
        cursor_y = 24;
    }
}

// Print a null-terminated string
void print(const char* str, uint8_t color) {
    for (int i = 0; str[i] != '\0'; i++) {
        putchar(str[i], color);
    }
}

// Ensure kernel_main is not name-mangled
#ifdef __cplusplus
extern "C" {
#endif

// Main kernel function - called from entry point
void kernel_main(void) {
    // Clear the screen with a blue background
    clear_screen(0x1F); // Light blue text on blue background

    // Print a welcome message
    print("Welcome to E-comOS Kernel!\n", 0x1F);

    // Initialize subsystems (placeholder for actual initialization)
    print("Initializing subsystems...\n", 0x1F);

    // Simulate initialization steps
    print("[OK] Memory Management Initialized\n", 0x2F);
    print("[OK] Scheduler Initialized\n", 0x2F);
    print("[OK] IPC Initialized\n", 0x2F);

    // Enter the main kernel loop
    print("Kernel is now running.\n", 0x1F);
    while (1) {
        // Simulate a simple task scheduler
        print("[Task Scheduler] Running tasks...\n", 0x1F);

        // Simulate task execution (placeholder for actual task handling)
        for (int i = 0; i < 3; i++) {
            print("[Task] Executing task ", 0x2F);
            putchar('1' + i, 0x2F);
            print("\n", 0x2F);
        }

        // Halt the CPU briefly to save power
        __asm__ volatile ("hlt");
    }
}


#ifdef __cplusplus
}
#endif
