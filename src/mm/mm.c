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

#include <kernel/mm.h>

#define MEMORY_START 0x100000  // 1MB
#define MEMORY_SIZE  0x1000000 // 16MB
#define MAX_PAGES    (MEMORY_SIZE / PAGE_SIZE)

static uint8_t page_bitmap[MAX_PAGES / 8];
static uint32_t next_free_page = 0;

void* mm_alloc_page(void) {
    for (uint32_t i = next_free_page; i < MAX_PAGES; i++) {
        uint32_t byte_idx = i / 8;
        uint32_t bit_idx = i % 8;
        
        if (!(page_bitmap[byte_idx] & (1 << bit_idx))) {
            page_bitmap[byte_idx] |= (1 << bit_idx);
            next_free_page = i + 1;
            return (void*)(MEMORY_START + i * PAGE_SIZE);
        }
    }
    return 0; // Out of memory
}

void mm_free_page(void* page) {
    uint32_t addr = (uint32_t)page;
    if (addr < MEMORY_START) return;
    
    uint32_t page_idx = (addr - MEMORY_START) / PAGE_SIZE;
    if (page_idx >= MAX_PAGES) return;
    
    uint32_t byte_idx = page_idx / 8;
    uint32_t bit_idx = page_idx % 8;
    
    page_bitmap[byte_idx] &= ~(1 << bit_idx);
    if (page_idx < next_free_page) {
        next_free_page = page_idx;
    }
}

int mm_map_page(uint32_t virtual_addr, uint32_t physical_addr, uint32_t flags) {
    (void)virtual_addr;
    (void)physical_addr;
    (void)flags;
    
    // Simple identity mapping for now
    // TODO: Implement proper page table management
    return 0;
}