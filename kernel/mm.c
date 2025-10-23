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
// Handles address space isolation between kernel and user-space services
#include "kernel.h"
#include <stddef.h>

// Global PML4 table (kernel's page table, stored in kernel memory)
static page_table_entry_t* kernel_pml4;

// Helper: Allocate a physical page (simplified: use pre-reserved kernel memory)
// Return: Physical address of the new page (0 on failure)
static void* mm_alloc_page() {
    // For initial version: Reserve 1MB-2MB as page pool (avoids BIOS-used memory)
    static uint64_t page_pool = 0x100000; // Start at 1MB
    void* phys_addr = (void*)page_pool;
    
    // Check if pool is exhausted (temporary limit: up to 2MB)
    if (page_pool >= 0x200000) {
        return NULL;
    }
    
    // Increment pool (4KB per page)
    page_pool += PAGE_SIZE;
    return phys_addr;
}

// Helper: Create a new page table (PML4/PDPT/PD/PT)
// Return: Page table entry pointer (virtual address)
static page_table_entry_t* mm_create_table() {
    void* phys_addr = mm_alloc_page();
    if (!phys_addr) {
        return NULL;
    }
    
    // Map physical page to kernel virtual address (simplified: 1:1 mapping for initial pages)
    void* virt_addr = (void*)((uint64_t)phys_addr + KERNEL_VIRT_BASE);
    
    // Zero-initialize the table (avoid garbage values)
    for (int i = 0; i < (PAGE_SIZE / sizeof(page_table_entry_t)); i++) {
        ((page_table_entry_t*)virt_addr)[i] = (page_table_entry_t){0};
    }
    
    return (page_table_entry_t*)virt_addr;
}

// Map a virtual address to physical address in a page table
// Parameters:
//   virt_addr: Virtual address to map
//   phys_addr: Physical address to map to
//   user: 1 = allow user-space access; 0 = kernel-only
void mm_map_page(void* virt_addr, void* phys_addr, int user) {
    if (!kernel_pml4) {
        return; // PML4 not initialized yet
    }
    
    // Calculate offsets for 4-level page table (x86_64)
    uint64_t virt = (uint64_t)virt_addr;
    uint64_t pml4_idx = (virt >> 39) & 0x1FF; // PML4 index (bits 39-47)
    uint64_t pdpt_idx = (virt >> 30) & 0x1FF; // PDPT index (bits 30-38)
    uint64_t pd_idx   = (virt >> 21) & 0x1FF; // PD index (bits 21-29)
    uint64_t pt_idx   = (virt >> 12) & 0x1FF; // PT index (bits 12-20)
    
    // Step 1: Get PDPT from PML4
    page_table_entry_t* pdpt_entry = &kernel_pml4[pml4_idx];
    if (!pdpt_entry->present) {
        // Create new PDPT if not exists
        page_table_entry_t* new_pdpt = mm_create_table();
        if (!new_pdpt) {
            return;
        }
        // Update PML4 entry (physical address = virt - KERNEL_VIRT_BASE)
        pdpt_entry->frame = ((uint64_t)new_pdpt - KERNEL_VIRT_BASE) >> 12;
        pdpt_entry->present = 1;
        pdpt_entry->writable = 1;
        pdpt_entry->user = user;
    }
    page_table_entry_t* pdpt = (page_table_entry_t*)((pdpt_entry->frame << 12) + KERNEL_VIRT_BASE);
    
    // Step 2: Get PD from PDPT
    page_table_entry_t* pd_entry = &pdpt[pdpt_idx];
    if (!pd_entry->present) {
        page_table_entry_t* new_pd = mm_create_table();
        if (!new_pd) {
            return;
        }
        pd_entry->frame = ((uint64_t)new_pd - KERNEL_VIRT_BASE) >> 12;
        pd_entry->present = 1;
        pd_entry->writable = 1;
        pd_entry->user = user;
    }
    page_table_entry_t* pd = (page_table_entry_t*)((pd_entry->frame << 12) + KERNEL_VIRT_BASE);
    
    // Step 3: Get PT from PD
    page_table_entry_t* pt_entry = &pd[pd_idx];
    if (!pt_entry->present) {
        page_table_entry_t* new_pt = mm_create_table();
        if (!new_pt) {
            return;
        }
        pt_entry->frame = ((uint64_t)new_pt - KERNEL_VIRT_BASE) >> 12;
        pt_entry->present = 1;
        pt_entry->writable = 1;
        pt_entry->user = user;
    }
    page_table_entry_t* pt = (page_table_entry_t*)((pt_entry->frame << 12) + KERNEL_VIRT_BASE);
    
    // Step 4: Map final page (PT entry)
    page_table_entry_t* page_entry = &pt[pt_idx];
    page_entry->frame = (uint64_t)phys_addr >> 12;
    page_entry->present = 1;
    page_entry->writable = 1;
    page_entry->user = user;
}

// Initialize memory management (create kernel PML4, map critical pages)
void mm_init() {
    // 1. Create kernel's PML4 table
    kernel_pml4 = mm_create_table();
    if (!kernel_pml4) {
        vga_put_char(0, 2, 'M', 0x04); // Red 'M' for memory init fail
        vga_put_char(1, 2, 'M', 0x04);
        vga_put_char(2, 2, ' ', 0x04);
        vga_put_char(3, 2, 'F', 0x04);
        vga_put_char(4, 2, 'a', 0x04);
        vga_put_char(5, 2, 'i', 0x04);
        vga_put_char(6, 2, 'l', 0x04);
        while (1); // Halt on critical error
    }
    
    // 2. Map kernel's own code/data to virtual address (1:1 mapping)
    // Kernel physical range: 0x100000 (loaded by DOS25) ~ 0x200000 (page pool end)
    for (uint64_t phys = 0x100000; phys < 0x200000; phys += PAGE_SIZE) {
        void* virt = (void*)(phys + KERNEL_VIRT_BASE);
        mm_map_page(virt, (void*)phys, 0); // 0 = kernel-only access
    }
    
    // 3. Map VGA buffer to kernel virtual address (for kernel VGA functions)
    mm_map_page((void*)(VGA_BUFFER + KERNEL_VIRT_BASE), (void*)VGA_BUFFER, 0);
    
    // 4. Load PML4 table (update CR3 register)
    uint64_t pml4_phys = (uint64_t)kernel_pml4 - KERNEL_VIRT_BASE;
    asm volatile ("mov %0, %%cr3" : : "r"(pml4_phys));
    
    vga_put_char(0, 2, 'M', VGA_COLOR); // Show 'MM Init OK'
    vga_put_char(1, 2, 'M', VGA_COLOR);
    vga_put_char(2, 2, ' ', VGA_COLOR);
    vga_put_char(3, 2, 'I', VGA_COLOR);
    vga_put_char(4, 2, 'n', VGA_COLOR);
    vga_put_char(5, 2, 'i', VGA_COLOR);
    vga_put_char(6, 2, 't', VGA_COLOR);
    vga_put_char(7, 2, ' ', VGA_COLOR);
    vga_put_char(8, 2, 'O', VGA_COLOR);
    vga_put_char(9, 2, 'K', VGA_COLOR);
}