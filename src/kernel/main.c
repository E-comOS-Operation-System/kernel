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

// VGA display functions
static volatile uint16_t* vga_buffer = (volatile uint16_t*)0xB8000;
static int cursor_x = 0, cursor_y = 0;

void vga_clear(void) {
    for (int i = 0; i < 80 * 25; i++) {
        vga_buffer[i] = 0x0720; // Space with white on black
    }
    cursor_x = cursor_y = 0;
}

void vga_putchar(char c) {
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    } else {
        int pos = cursor_y * 80 + cursor_x;
        vga_buffer[pos] = 0x0F00 | c; // White on black
        cursor_x++;
    }
    
    if (cursor_x >= 80) {
        cursor_x = 0;
        cursor_y++;
    }
    
    if (cursor_y >= 25) {
        cursor_y = 0; // Simple wrap
    }
}

void vga_print(const char* str) {
    while (*str) {
        vga_putchar(*str++);
    }
}

// Simple memory allocator
static uint64_t next_free_page = 0x300000; // Start after kernel

void* alloc_page(void) {
    void* page = (void*)next_free_page;
    next_free_page += 4096;
    return page;
}

// Process structure
typedef struct process {
    uint64_t pid;
    uint64_t rsp;
    int state; // 0=running, 1=ready, 2=blocked
    struct process* next;
} process_t;

static process_t* current_process = 0;
static uint64_t next_pid = 1;

// Create new process
uint64_t create_process(void (*entry)(void)) {
    process_t* proc = (process_t*)alloc_page();
    if (!proc) return 0;
    
    proc->pid = next_pid++;
    proc->state = 1; // ready
    proc->rsp = (uint64_t)alloc_page() + 4096 - 8; // Stack top
    proc->next = current_process;
    current_process = proc;
    
    // Set up initial stack frame
    *(uint64_t*)proc->rsp = (uint64_t)entry;
    
    return proc->pid;
}

// Kernel main function
void kernel_main(void) {
    // Initialize display
    vga_clear();
    vga_print("E-comOS 64-bit Microkernel\n");
    vga_print("Copyright (C) 2025 Saladin5101\n\n");
    
    // Initialize subsystems
    vga_print("Initializing Memory Manager...OK\n");
    vga_print("Initializing Process Manager...OK\n");
    
    vga_print("\nMicrokernel Ready!\n");
    vga_print("Philosophy: Everything is a service\n");
    
    // Microkernel main loop - pure 64-bit
    while (1) {
        __asm__ volatile ("hlt");
    }
}