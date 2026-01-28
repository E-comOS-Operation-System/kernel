/* E-comOS Kernel - A Microkernel for E-comOS
   Copyright (C) 2025,2026 Saladin5101

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

#define PAGE_SIZE 4096
#define MEMORY_START 0x100000 // 1MB
#define MEMORY_SIZE 0x1000000 // 16MB
#define MAX_PAGES (MEMORY_SIZE / PAGE_SIZE)

// Page table management definitions
#define PAGE_DIRECTORY_ENTRIES 1024
#define PAGE_TABLE_ENTRIES 1024

// Page table entry flags
#define PTE_PRESENT   (1 << 0)
#define PTE_WRITABLE  (1 << 1)
#define PTE_USER      (1 << 2)

// Page directory entry flags (same as PTE for simplicity)
#define PDE_PRESENT   PTE_PRESENT
#define PDE_WRITABLE  PTE_WRITABLE
#define PDE_USER      PTE_USER

static uint8_t page_bitmap[MAX_PAGES / 8];
static uint32_t next_free_page = 0;

// Page table management variables
static uint32_t* page_directory = 0;
static uint32_t* page_tables[4] = {0}; // 4 page tables for 16MB address space
static int page_tables_initialized = 0;

// Initialize page tables for identity mapping
static int init_page_tables(void) {
    if (page_tables_initialized) {
        return 0;
    }
    
    // Allocate page directory
    page_directory = (uint32_t*)mm_alloc_page();
    if (!page_directory) {
        return -1; // Out of memory
    }
    
    // Initialize page directory entries
    for (int i = 0; i < PAGE_DIRECTORY_ENTRIES; i++) {
        page_directory[i] = 0; // Mark as not present initially
    }
    
    // Allocate and initialize page tables for first 16MB
    for (int i = 0; i < 4; i++) {
        page_tables[i] = (uint32_t*)mm_alloc_page();
        if (!page_tables[i]) {
            return -1; // Out of memory
        }
        
        // Clear page table
        for (int j = 0; j < PAGE_TABLE_ENTRIES; j++) {
            page_tables[i][j] = 0;
        }
        
        // Set up identity mapping for this 4MB region
        uint32_t base_addr = i * 4 * 1024 * 1024; // 4MB per page table
        for (int j = 0; j < PAGE_TABLE_ENTRIES; j++) {
            uint32_t phys_addr = base_addr + j * PAGE_SIZE;
            
            // Only map physical pages that are within our managed memory
            if (phys_addr >= MEMORY_START && phys_addr < MEMORY_START + MEMORY_SIZE) {
                page_tables[i][j] = phys_addr | PTE_PRESENT | PTE_WRITABLE;
            }
        }
        
        // Set page directory entry
        uint32_t page_table_phys = (uint32_t)page_tables[i];
        page_directory[i] = page_table_phys | PDE_PRESENT | PDE_WRITABLE;
    }
    
    page_tables_initialized = 1;
    return 0;
}

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
    // Initialize page tables if not already done
    if (!page_tables_initialized) {
        if (init_page_tables() != 0) {
            return -1; // Initialization failed
        }
    }
    
    // Validate addresses
    if (physical_addr < MEMORY_START || physical_addr >= MEMORY_START + MEMORY_SIZE) {
        return -1; // Physical address out of range
    }
    
    if (virtual_addr >= 16 * 1024 * 1024) { // Only support first 16MB virtual address space
        return -1; // Virtual address out of range
    }
    
    // Calculate page directory index and page table index
    uint32_t page_dir_index = virtual_addr >> 22;        // Top 10 bits
    uint32_t page_table_index = (virtual_addr >> 12) & 0x3FF; // Next 10 bits
    
    // Check if we have a page table for this region
    if (page_dir_index >= 4 || !page_tables[page_dir_index]) {
        return -1; // Page table not available
    }
    
    // Set the page table entry
    uint32_t* page_table = page_tables[page_dir_index];
    page_table[page_table_index] = physical_addr | flags | PTE_PRESENT;
    
    return 0; // Success
}

// Helper function to get page directory physical address for CR3
uint32_t mm_get_page_directory(void) {
    if (!page_tables_initialized) {
        if (init_page_tables() != 0) {
            return 0;
        }
    }
    return (uint32_t)page_directory;
}

// Helper function to enable paging (should be called from architecture-specific code)
void mm_enable_paging(void) {
    uint32_t cr3 = mm_get_page_directory();
    if (cr3) {
        __asm__ volatile (
            "mov %0, %%cr3\n"
            "mov %%cr0, %%eax\n"
            "or $0x80000000, %%eax\n" // Set PG bit
            "mov %%eax, %%cr0\n"
            :
            : "r"(cr3)
            : "eax", "memory"
        );
    }
}