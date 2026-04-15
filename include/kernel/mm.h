/*
    E-comOS Kernel - Memory Manager Interface
    Copyright (C) 2025,2026  Saladin5101

    Invariant: pageBitmap bit i == 1  ↔  physical page i is allocated.
    Invariant: nextFreePage ≤ MAX_PAGES at all times.
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
} MemoryStatus;

/*
 * mmInit — initialise the physical page allocator.
 *
 * Precondition:  called exactly once, before any mmAllocPage call.
 * Precondition:  interrupts are disabled.
 * Postcondition: pageBitmap reflects all usable physical pages;
 *                kernel image pages are marked allocated.
 * Postcondition: nextFreePage points to the first free page index,
 *                or equals MAX_PAGES if no free pages exist.
 *
 * bootParams may be NULL; in that case a conservative fallback is used.
 */
MemoryStatus mmInit(BootParams *bootParams);

/*
 * mmAllocPage — allocate one physical page (4 KB).
 * Returns physical address, or NULL if OOM.
 * NOT interrupt-safe; caller must disable interrupts if needed.
 */
void *mmAllocPage(void);

/* mmFreePage — release a page previously returned by mmAllocPage. */
void  mmFreePage(void *page);

/*
 * mmMapPage — insert a vaddr→paddr mapping into the current page tables.
 * flags: combination of PTE_* constants.
 */
int   mmMapPage(uint32_t vaddr, uint32_t paddr, uint32_t flags);
int   mmUnmapPage(uint32_t vaddr);

/*
 * mmEnablePaging — load CR3 and set CR0.PG.
 * Precondition: page tables built by initPageTables() (called from mmInit).
 * Panics if page tables are not ready.
 */
void  mmEnablePaging(void);

extern uint8_t  pageBitmap[MAX_PAGES / 8];
extern uint32_t nextFreePage;

#endif
