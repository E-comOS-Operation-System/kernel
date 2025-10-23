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
#ifndef KERNEL_H
#define KERNEL_H

#include <stdint.h>
extern page_table_entry_t* kernel_pml4;
// -------------------------- Hardware Related Definitions --------------------------
#define VGA_BUFFER 0xB8000          // VGA text mode buffer physical address
#define VGA_WIDTH 80                // VGA Screen width (in characters)
#define VGA_HEIGHT 25               // VGA Screen height (in characters)

// -------------------------- Memory-related Definitions --------------------------
#define PAGE_SIZE 4096              // Page size（4KB）
#define KERNEL_VIRT_BASE 0xFFFFFFFF80000000  // Kernel virtual address base address (to avoid conflicts with user mode)

// Page list entry structure (64-bit)
typedef struct {
    uint64_t present : 1;    // Is page present in memory
    uint64_t writable : 1;   // Is page can written
    uint64_t user : 1;       // Is allow user-mode access
    uint64_t reserved : 9;   // Reserved bits
    uint64_t frame : 40;     // Physical frame address (shifted right 12 bits)
    uint64_t reserved2 : 12; // Reserved bits
} page_table_entry_t;

// --------------------------Function Declaration--------------------------
// Kernel entry point (DOS25 will jump to here)
void kernel_main();
// VGA helper functions
void vga_put_char(int x, int y, char c, uint8_t color);
void vga_clear_screen(uint8_t color);
// Mentory management functions
void mm_init();
// Created a new page table
void mm_map_page(void* virt_addr, void* phys_addr, int user);
// Scheduler functions
void sched_init();
// Create a new task (user or kernel)
uint64_t sched_create_task(void (*entry)(), int is_user);
void sched_switch_task();
#endif