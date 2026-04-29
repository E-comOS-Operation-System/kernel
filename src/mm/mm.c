/*
    E-comOS Kernel - Memory Manager
    Copyright (C) 2025,2026  Saladin5101
    Lincesed on AGPL Version 3.
    
    Physical memory window managed: [PHYS_BASE, PHYS_BASE + PHYS_SIZE)
    = [0x100000, 0x1100000)  (1 MB … 17 MB, 4096 × 4 KB pages)

    Page table layout (64-bit, 4-level, identity-mapped):
      PML4[0] → PDPT[0] → PD[0..3] → PT[0..1023]  (covers 0 … 16 MB)
*/

#include <kernel/mm.h>
#include <kernel/boot.h>
#include <kernel/printkit/print.h>
#include <stdint.h>

/* ------------------------------------------------------------------ */
/* Constants                                                           */
/* ------------------------------------------------------------------ */
#define PHYS_BASE  0x100000ULL          /* first managed physical page  */
#define PHYS_SIZE  (MAX_PAGES * (uint64_t)PAGE_SIZE)  /* 16 MB          */

/* Number of pages to reserve for the kernel image.
 * Computed precisely from linker symbols at runtime; this is the
 * upper bound used before mm_init has run (should never be needed). */
#define KERNEL_RESERVED_PAGES_FALLBACK 64u  /* 256 KB conservative bound */

/* ------------------------------------------------------------------ */
/* Global allocator state                                              */
/* ------------------------------------------------------------------ */
uint8_t  page_bitmap[MAX_PAGES / 8] = {0};
uint32_t next_free_page = 0;

/* ------------------------------------------------------------------ */
/* 64-bit page table structures (4-level paging, identity map)        */
/* ------------------------------------------------------------------ */
/*
 * We need: PML4 (1 entry used) → PDPT (1 entry used) → PD (4 entries)
 * → 4 × PT (1024 entries each).
 * All structures are 4 KB aligned and stored in BSS so they are
 * zero-initialised before mm_init runs.
 *
 * Entry format (bits):
 *   [0]   Present
 *   [1]   Read/Write
 *   [2]   User/Supervisor
 *   [12+] Physical address of next-level table (4 KB aligned)
 */
#define PT_ENTRIES  512u   /* 64-bit PT has 512 × 8-byte entries        */
#define PD_ENTRIES  512u
#define PDPT_ENTRIES 512u
#define PML4_ENTRIES 512u

/* Each PT covers 512 × 4 KB = 2 MB.  We need 8 PTs for 16 MB. */
#define NUM_PTS 8u

static uint64_t pml4[PML4_ENTRIES]  __attribute__((aligned(PAGE_SIZE)));
static uint64_t pdpt[PDPT_ENTRIES]  __attribute__((aligned(PAGE_SIZE)));
static uint64_t pd[PD_ENTRIES]      __attribute__((aligned(PAGE_SIZE)));
static uint64_t pt[NUM_PTS][PT_ENTRIES] __attribute__((aligned(PAGE_SIZE)));

static int page_tables_ready = 0;

/* ------------------------------------------------------------------ */
/* Panic helper (no dependency on heap)                               */
/* ------------------------------------------------------------------ */
static void __attribute__((noreturn)) mm_panic(const char *msg) {
    print_str("MM PANIC: ", 0x4F);
    print_str(msg, 0x4F);
    __asm__ volatile("cli");
    while (1) __asm__ volatile("hlt");
}

/* ------------------------------------------------------------------ */
/* Bitmap helpers                                                      */
/* ------------------------------------------------------------------ */
static inline void bitmap_set(uint32_t idx) {
    page_bitmap[idx >> 3] |= (uint8_t)(1u << (idx & 7u));
}

static inline void bitmap_clear(uint32_t idx) {
    page_bitmap[idx >> 3] &= (uint8_t)~(1u << (idx & 7u));
}

static inline int bitmap_test(uint32_t idx) {
    return (page_bitmap[idx >> 3] >> (idx & 7u)) & 1u;
}

/* ------------------------------------------------------------------ */
/* 64-bit identity page table setup                                   */
/* ------------------------------------------------------------------ */
/*
 * Builds a 4-level identity map covering [0, NUM_PTS × 2 MB).
 * Called from mm_init after the bitmap is ready so that mm_alloc_page
 * is NOT used here — page table memory is statically allocated.
 *
 * Postcondition: pml4/pdpt/pd/pt are fully populated;
 *                page_tables_ready == 1.
 */
static void build_page_tables(void) {
    /* PML4[0] → pdpt */
    pml4[0] = (uint64_t)(uintptr_t)pdpt | PTE_PRESENT | PTE_WRITABLE;

    /* PDPT[0] → pd */
    pdpt[0] = (uint64_t)(uintptr_t)pd | PTE_PRESENT | PTE_WRITABLE;

    /* PD[i] → pt[i]  (each covers 2 MB) */
    for (uint32_t i = 0; i < NUM_PTS; i++) {
        pd[i] = (uint64_t)(uintptr_t)pt[i] | PTE_PRESENT | PTE_WRITABLE;

        /* Fill PT: identity-map 512 × 4 KB pages */
        for (uint32_t j = 0; j < PT_ENTRIES; j++) {
            uint64_t phys = (uint64_t)i * PT_ENTRIES * PAGE_SIZE
                          + (uint64_t)j * PAGE_SIZE;
            pt[i][j] = phys | PTE_PRESENT | PTE_WRITABLE;
        }
    }

    page_tables_ready = 1;
}

/* ------------------------------------------------------------------ */
/* mmInit                                                              */
/* ------------------------------------------------------------------ */
/*
 * Precondition:  interrupts disabled.
 * Precondition:  called exactly once.
 * Postcondition: page_bitmap valid; page_tables_ready == 1.
 */
memory_status mm_init(boot_params *boot_params) {
    /* Step 1: mark everything allocated (deny-by-default) */
    for (uint32_t i = 0; i < MAX_PAGES / 8u; i++)
        page_bitmap[i] = 0xFFu;
    next_free_page = MAX_PAGES; /* sentinel: no free pages yet */

    /* Step 2: determine kernel image extent from linker symbols */
    uint64_t kern_start = (uint64_t)(uintptr_t)_kernelStart;
    uint64_t kern_end   = (uint64_t)(uintptr_t)_kernelEnd;

    /* Step 3: parse UEFI memory map or use fallback */
    if (!boot_params
            || !boot_params->memory_map
            || boot_params->memory_map_size == 0
            || boot_params->memory_map_descriptor_size < sizeof(efi_memory_descriptor)) {
        /*
         * Fallback: assume the entire managed window is conventional RAM
         * except the kernel image pages.  Used on bare QEMU without a
         * proper UEFI loader.
         */
        for (uint32_t i = 0; i < MAX_PAGES; i++)
            bitmap_clear(i);

        /* Re-mark kernel pages as used */
        if (kern_start >= PHYS_BASE && kern_end > kern_start) {
            uint32_t k_first = (uint32_t)((kern_start - PHYS_BASE) / PAGE_SIZE);
            uint32_t k_last  = (uint32_t)((kern_end   - PHYS_BASE + PAGE_SIZE - 1u)
                                          / PAGE_SIZE);
            if (k_last > MAX_PAGES) k_last = MAX_PAGES;
            for (uint32_t i = k_first; i < k_last; i++)
                bitmap_set(i);
        } else {
            /* Linker symbols unavailable — use conservative bound */
            for (uint32_t i = 0; i < KERNEL_RESERVED_PAGES_FALLBACK; i++)
                bitmap_set(i);
        }

        print_str("MM: fallback map (no UEFI params)\n", 0x0E);
        goto find_first;
    }

    /* Step 4: walk UEFI memory map */
    {
        const uint8_t *base     = (const uint8_t *)boot_params->memory_map;
        uint64_t       stride   = boot_params->memory_map_descriptor_size;
        uint64_t       num_descs = boot_params->memory_map_size / stride;

        for (uint64_t d = 0; d < num_descs; d++) {
            const efi_memory_descriptor *desc =
                (const efi_memory_descriptor *)(base + d * stride);

            if (desc->type != EFI_CONVENTIONAL_MEMORY)
                continue;

            uint64_t region_start = desc->physical_start;
            uint64_t region_pages = desc->number_of_pages;

            /* Overflow guard: skip absurdly large descriptors */
            if (region_pages > (PHYS_SIZE / PAGE_SIZE))
                region_pages = PHYS_SIZE / PAGE_SIZE;

            for (uint64_t p = 0; p < region_pages; p++) {
                uint64_t phys = region_start + p * (uint64_t)PAGE_SIZE;

                if (phys < PHYS_BASE)
                    continue;
                if (phys >= PHYS_BASE + PHYS_SIZE)
                    break; /* rest of this descriptor is outside window */

                uint32_t idx = (uint32_t)((phys - PHYS_BASE) / PAGE_SIZE);
                /* idx < MAX_PAGES guaranteed by the range check above */
                bitmap_clear(idx);
            }
        }
    }

    /* Step 5: re-mark kernel image pages as used */
    if (kern_start >= PHYS_BASE && kern_end > kern_start) {
        uint32_t k_first = (uint32_t)((kern_start - PHYS_BASE) / PAGE_SIZE);
        uint32_t k_last  = (uint32_t)((kern_end - PHYS_BASE + PAGE_SIZE - 1u)
                                      / PAGE_SIZE);
        if (k_last > MAX_PAGES) k_last = MAX_PAGES;
        for (uint32_t i = k_first; i < k_last; i++)
            bitmap_set(i);
    } else {
        for (uint32_t i = 0; i < KERNEL_RESERVED_PAGES_FALLBACK; i++)
            bitmap_set(i);
    }

find_first:
    /* Step 6: find first free page */
    next_free_page = MAX_PAGES; /* assume none */
    for (uint32_t i = 0; i < MAX_PAGES; i++) {
        if (!bitmap_test(i)) {
            next_free_page = i;
            break;
        }
    }

    /* Step 7: build 64-bit page tables (static memory, no alloc needed) */
    build_page_tables();

    /* Step 8: report */
    uint32_t free_count = 0;
    for (uint32_t i = 0; i < MAX_PAGES; i++)
        if (!bitmap_test(i)) free_count++;

    print_str("MM: free pages: ", 0x0A);
    print_num(free_count, 0x0A);
    print_str(" / ", 0x0A);
    print_num(MAX_PAGES, 0x0A);
    print_str("  first free: ", 0x0A);
    print_num(next_free_page, 0x0A);
    print_str("\n", 0x0A);

    if (free_count == 0)
        return MEMORY_ERROR_NOMEM;

    return MEMORY_SUCCESS;
}

/* ------------------------------------------------------------------ */
/* mmAllocPage                                                         */
/* ------------------------------------------------------------------ */
/*
 * NOT interrupt-safe.  Caller must disable interrupts if called after
 * sti, or use an external lock.
 */
void *mm_alloc_page(void) {
    for (uint32_t i = next_free_page; i < MAX_PAGES; i++) {
        if (!bitmap_test(i)) {
            bitmap_set(i);
            next_free_page = i + 1u;
            return (void *)(uintptr_t)(PHYS_BASE + (uint64_t)i * PAGE_SIZE);
        }
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/* mmFreePage                                                          */
/* ------------------------------------------------------------------ */
void mm_free_page(void *page) {
    uint64_t addr = (uint64_t)(uintptr_t)page;
    if (addr < PHYS_BASE || addr >= PHYS_BASE + PHYS_SIZE)
        return;
    if (addr & (PAGE_SIZE - 1u))
        return; /* not page-aligned — refuse silently */
    uint32_t idx = (uint32_t)((addr - PHYS_BASE) / PAGE_SIZE);
    bitmap_clear(idx);
    if (idx < next_free_page)
        next_free_page = idx;
}

/* ------------------------------------------------------------------ */
/* mmMapPage                                                           */
/* ------------------------------------------------------------------ */
int mm_map_page(uint32_t vaddr, uint32_t paddr, uint32_t flags) {
    if (!page_tables_ready)
        return -1;

    uint32_t pd_idx  = (vaddr >> 21) & 0x1FFu; /* PD index (2 MB granule) */
    uint32_t pt_idx  = (vaddr >> 12) & 0x1FFu; /* PT index */

    if (pd_idx >= NUM_PTS)
        return -1; /* outside identity-mapped range */

    uint64_t entry = (uint64_t)paddr | PTE_PRESENT;
    if (flags & MM_FLAG_WRITE) entry |= PTE_WRITABLE;
    if (flags & MM_FLAG_USER)  entry |= PTE_USER;

    pt[pd_idx][pt_idx] = entry;

    /* Invalidate TLB entry */
    __asm__ volatile("invlpg (%0)" : : "r"((uintptr_t)vaddr) : "memory");
    return 0;
}

/* ------------------------------------------------------------------ */
/* mmUnmapPage                                                         */
/* ------------------------------------------------------------------ */
int mm_unmap_page(uint32_t vaddr) {
    if (!page_tables_ready)
        return -1;
    uint32_t pd_idx = (vaddr >> 21) & 0x1FFu;
    uint32_t pt_idx = (vaddr >> 12) & 0x1FFu;
    if (pd_idx >= NUM_PTS)
        return -1;
    pt[pd_idx][pt_idx] = 0;
    __asm__ volatile("invlpg (%0)" : : "r"((uintptr_t)vaddr) : "memory");
    return 0;
}

/* ------------------------------------------------------------------ */
/* mmEnablePaging                                                      */
/* ------------------------------------------------------------------ */
/*
 * Precondition: build_page_tables() has been called (page_tables_ready == 1).
 * Panics if page tables are not ready — silent failure here would cause
 * undefined behaviour on the first memory access after return.
 *
 * IMPORTANT: This function assumes that PAE (Physical Address Extension)
 * and LME (Long Mode Enable) have already been set in the assembly
 * boot code (long_mode_switch.s). It only loads CR3 and sets the PG bit.
 */
void mm_enable_paging(void) {
    if (!page_tables_ready)
        mm_panic("mmEnablePaging called before page tables are built");

    uint64_t cr3 = (uint64_t)(uintptr_t)pml4;
    uint64_t pae_bit = 0x20ULL;
    uint64_t pg_bit = 0x80000000ULL;
    
    // Enable PAE
    __asm__ volatile(
        "movq %0, %%cr3\n"        // Load CR3 first
        :
        : "r"(cr3)
        : "memory"
    );
    
    __asm__ volatile(
        "movq %%cr4, %%rax\n"
        "orq  %0, %%rax\n"
        "movq %%rax, %%cr4\n"
        :
        : "r"(pae_bit)
        : "rax"
    );
    
    // Enable paging
    __asm__ volatile(
        "movq %%cr0, %%rax\n"
        "orq  %0, %%rax\n"
        "movq %%rax, %%cr0\n"
        :
        : "r"(pg_bit)
        : "rax", "memory"
    );
}
