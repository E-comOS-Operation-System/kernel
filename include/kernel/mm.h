/*
    E-comOS Kernel - Memory Manager Interface
    Copyright (C) 2025,2026  Saladin5101

    Invariant: page_bitmap bit i == 1  ↔  physical page i is allocated.
    Invariant: next_free_page ≤ MAX_PAGES at all times.
*/

#ifndef KERNEL_MM_H
#define KERNEL_MM_H

#include <stdint.h>
#include <kernel/boot.h>

#define PAGE_SIZE       4096u
#define MAX_PHYS_PAGES  4096u   /* manages 16 MB: [0x100000, 0x1100000) */
#define MAX_PAGES       MAX_PHYS_PAGES
#define KERNEL_BASE     0x100000u

/* x86 page-table entry flags */
#define PTE_PRESENT  (1u << 0)
#define PTE_WRITABLE (1u << 1)
#define PTE_USER     (1u << 2)

/* Higher-level mapping flags (translated to PTE flags by mmMapPage) */
#define MM_FLAG_READ    (1u << 0)
#define MM_FLAG_WRITE   (1u << 1)
#define MM_FLAG_EXEC    (1u << 2)
#define MM_FLAG_USER    (1u << 3)
#define MM_FLAG_DEVICE  (1u << 4)
#define MM_FLAG_CACHED  (1u << 5)

#define MM_FLAG_KERNEL_RW (MM_FLAG_READ | MM_FLAG_WRITE)
#define MM_FLAG_USER_RO   (MM_FLAG_READ | MM_FLAG_USER)
#define MM_FLAG_USER_RW   (MM_FLAG_READ | MM_FLAG_WRITE | MM_FLAG_USER)

typedef enum {
    MEMORY_SUCCESS              =  0,
    MEMORY_ERROR_INVALID_PARAMS = -1,
    MEMORY_ERROR_NOMEM          = -2,
    MEMORY_ERROR_BUSY           = -3
} memory_status;

/*
 * mm_init — initialise the physical page allocator.
 *
 * Precondition:  called exactly once, before any mm_alloc_page call.
 * Precondition:  interrupts are disabled.
 * Postcondition: page_bitmap reflects all usable physical pages;
 *                kernel image pages are marked allocated.
 * Postcondition: next_free_page points to the first free page index,
 *                or equals MAX_PAGES if no free pages exist.
 *
 * boot_params may be NULL; in that case a conservative fallback is used.
 */
memory_status mm_init(boot_params *boot_params);

/*
 * mm_alloc_page — allocate one physical page (4 KB).
 * Returns physical address, or NULL if OOM.
 * NOT interrupt-safe; caller must disable interrupts if needed.
 */
void *mm_alloc_page(void);

/* mmFreePage — release a page previously returned by mmAllocPage. */
void  mm_free_page(void *page);

/*
 * mm_map_page — insert a vaddr→paddr mapping into the current page tables.
 * flags: combination of PTE_* constants.
 */
int   mm_map_page(uint32_t vaddr, uint32_t paddr, uint32_t flags);
int   mm_unmap_page(uint32_t vaddr);

/*
 * mm_enable_paging — load CR3 and set CR0.PG.
 * Precondition: page tables built by init_page_tables() (called from mm_init).
 * Panics if page tables are not ready.
 */
void  mm_enable_paging(void);

extern uint8_t  page_bitmap[MAX_PAGES / 8];
extern uint32_t next_free_page;

#endif
