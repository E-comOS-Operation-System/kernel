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

#ifndef KERNEL_MM_H
#define KERNEL_MM_H

#include <stdint.h>

/* Memory management configuration constants */
#define PAGE_SIZE 4096                    // Page size in bytes (4KB)
#define MAX_PHYS_PAGES 4096               // Maximum number of physical pages (16MB memory)
#define KERNEL_BASE 0x100000              // Kernel base address (1MB)

/* Memory mapping flags */
#define MM_FLAG_READ    (1 << 0)          // Read permission
#define MM_FLAG_WRITE   (1 << 1)          // Write permission
#define MM_FLAG_EXEC    (1 << 2)          // Execute permission
#define MM_FLAG_USER    (1 << 3)          // User space access
#define MM_FLAG_DEVICE  (1 << 4)          // Device memory mapping
#define MM_FLAG_CACHED  (1 << 5)          // Cache enabled

/* Combined flag definitions */
#define MM_FLAG_KERNEL_RW (MM_FLAG_READ | MM_FLAG_WRITE)     // Kernel read-write permissions
#define MM_FLAG_USER_RO   (MM_FLAG_READ | MM_FLAG_USER)      // User read-only permissions
#define MM_FLAG_USER_RW   (MM_FLAG_READ | MM_FLAG_WRITE | MM_FLAG_USER) // User read-write permissions

/* Error code definitions */
#define MM_SUCCESS      0                 // Operation successful
#define MM_ERROR_NOMEM  -1                // Out of memory
#define MM_ERROR_INVALID -2               // Invalid parameter
#define MM_ERROR_BUSY   -3                // Resource busy

/**
 * Allocates a physical memory page
 * 
 * @return Returns physical address of allocated page on success, NULL on failure
 * @note The allocated page is uninitialized, content is undefined
 */
void* mm_alloc_page(void);

/**
 * Frees a previously allocated physical memory page
 * 
 * @param page Physical address of the page to free
 * @note The address must have been previously allocated by mm_alloc_page
 */
void mm_free_page(void* page);

/**
 * Establishes mapping from virtual address to physical address
 * 
 * @param virtual_addr Virtual address (must be page-aligned)
 * @param physical_addr Physical address (must be page-aligned)
 * @param flags Mapping flags (combination of MM_FLAG_*)
 * @return Returns MM_SUCCESS on success, error code on failure
 */
int mm_map_page(uint32_t virtual_addr, uint32_t physical_addr, uint32_t flags);

/**
 * Unmaps a virtual address mapping
 * 
 * @param virtual_addr Virtual address to unmap
 * @return Returns MM_SUCCESS on success, error code on failure
 */
int mm_unmap_page(uint32_t virtual_addr);

/**
 * Queries mapping information for a virtual address
 * 
 * @param virtual_addr Virtual address to query
 * @param[out] physical_addr Returns mapped physical address
 * @param[out] flags Returns mapping flags
 * @return Returns MM_SUCCESS on success, error code on failure
 */
int mm_query_page(uint32_t virtual_addr, uint32_t* physical_addr, uint32_t* flags);

/**
 * Initializes the memory management system
 * 
 * @param memory_size Available physical memory size in bytes
 * @return Returns MM_SUCCESS on success, error code on failure
 */
int mm_init(uint32_t memory_size);

/* Compatibility definitions for existing code */
#define PTE_PRESENT   (1 << 0)    // Page present flag
#define PTE_WRITABLE  (1 << 1)    // Page writable flag
#define MAX_PAGES MAX_PHYS_PAGES
/* Paging function declarations */
void mm_enable_paging(void);

/* Global variable declarations */
extern uint8_t page_bitmap[];     // Page bitmap array

#endif /* KERNEL_MM_H */