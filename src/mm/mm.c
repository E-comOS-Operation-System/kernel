/*
    E-comOS Kernel - Memory Manager
    Copyright (C) 2025,2026  Saladin5101

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
 * upper bound used before mmInit has run (should never be needed). */
#define KERNEL_RESERVED_PAGES_FALLBACK 64u  /* 256 KB conservative bound */

/* ------------------------------------------------------------------ */
/* Global allocator state                                              */
/* ------------------------------------------------------------------ */
uint8_t  pageBitmap[MAX_PAGES / 8] = {0};
uint32_t nextFreePage = 0;

/* ------------------------------------------------------------------ */
/* 64-bit page table structures (4-level paging, identity map)        */
/* ------------------------------------------------------------------ */
/*
 * We need: PML4 (1 entry used) → PDPT (1 entry used) → PD (4 entries)
 * → 4 × PT (1024 entries each).
 * All structures are 4 KB aligned and stored in BSS so they are
 * zero-initialised before mmInit runs.
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

static int pageTablesReady = 0;

/* ------------------------------------------------------------------ */
/* Panic helper (no dependency on heap)                               */
/* ------------------------------------------------------------------ */
static void __attribute__((noreturn)) mmPanic(const char *msg) {
    printStr("MM PANIC: ", 0x4F);
    printStr(msg, 0x4F);
    __asm__ volatile("cli");
    while (1) __asm__ volatile("hlt");
}

/* ------------------------------------------------------------------ */
/* Bitmap helpers                                                      */
/* ------------------------------------------------------------------ */
static inline void bitmapSet(uint32_t idx) {
    pageBitmap[idx >> 3] |= (uint8_t)(1u << (idx & 7u));
}

static inline void bitmapClear(uint32_t idx) {
    pageBitmap[idx >> 3] &= (uint8_t)~(1u << (idx & 7u));
}

static inline int bitmapTest(uint32_t idx) {
    return (pageBitmap[idx >> 3] >> (idx & 7u)) & 1u;
}

/* ------------------------------------------------------------------ */
/* 64-bit identity page table setup                                   */
/* ------------------------------------------------------------------ */
/*
 * Builds a 4-level identity map covering [0, NUM_PTS × 2 MB).
 * Called from mmInit after the bitmap is ready so that mmAllocPage
 * is NOT used here — page table memory is statically allocated.
 *
 * Postcondition: pml4/pdpt/pd/pt are fully populated;
 *                pageTablesReady == 1.
 */
static void buildPageTables(void) {
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

    pageTablesReady = 1;
}

/* ------------------------------------------------------------------ */
/* mmInit                                                              */
/* ------------------------------------------------------------------ */
/*
 * Precondition:  interrupts disabled.
 * Precondition:  called exactly once.
 * Postcondition: pageBitmap valid; pageTablesReady == 1.
 */
MemoryStatus mmInit(BootParams *bootParams) {
    /* Step 1: mark everything allocated (deny-by-default) */
    for (uint32_t i = 0; i < MAX_PAGES / 8u; i++)
        pageBitmap[i] = 0xFFu;
    nextFreePage = MAX_PAGES; /* sentinel: no free pages yet */

    /* Step 2: determine kernel image extent from linker symbols */
    uint64_t kernStart = (uint64_t)(uintptr_t)_kernelStart;
    uint64_t kernEnd   = (uint64_t)(uintptr_t)_kernelEnd;

    /* Step 3: parse UEFI memory map or use fallback */
    if (!bootParams
            || !bootParams->memoryMap
            || bootParams->memoryMapSize == 0
            || bootParams->memoryMapDescriptorSize < sizeof(EfiMemoryDescriptor)) {
        /*
         * Fallback: assume the entire managed window is conventional RAM
         * except the kernel image pages.  Used on bare QEMU without a
         * proper UEFI loader.
         */
        for (uint32_t i = 0; i < MAX_PAGES; i++)
            bitmapClear(i);

        /* Re-mark kernel pages as used */
        if (kernStart >= PHYS_BASE && kernEnd > kernStart) {
            uint32_t kFirst = (uint32_t)((kernStart - PHYS_BASE) / PAGE_SIZE);
            uint32_t kLast  = (uint32_t)((kernEnd   - PHYS_BASE + PAGE_SIZE - 1u)
                                          / PAGE_SIZE);
            if (kLast > MAX_PAGES) kLast = MAX_PAGES;
            for (uint32_t i = kFirst; i < kLast; i++)
                bitmapSet(i);
        } else {
            /* Linker symbols unavailable — use conservative bound */
            for (uint32_t i = 0; i < KERNEL_RESERVED_PAGES_FALLBACK; i++)
                bitmapSet(i);
        }

        printStr("MM: fallback map (no UEFI params)\n", 0x0E);
        goto findFirst;
    }

    /* Step 4: walk UEFI memory map */
    {
        const uint8_t *base     = (const uint8_t *)bootParams->memoryMap;
        uint64_t       stride   = bootParams->memoryMapDescriptorSize;
        uint64_t       numDescs = bootParams->memoryMapSize / stride;

        for (uint64_t d = 0; d < numDescs; d++) {
            const EfiMemoryDescriptor *desc =
                (const EfiMemoryDescriptor *)(base + d * stride);

            if (desc->type != EFI_CONVENTIONAL_MEMORY)
                continue;

            uint64_t regionStart = desc->physicalStart;
            uint64_t regionPages = desc->numberOfPages;

            /* Overflow guard: skip absurdly large descriptors */
            if (regionPages > (PHYS_SIZE / PAGE_SIZE))
                regionPages = PHYS_SIZE / PAGE_SIZE;

            for (uint64_t p = 0; p < regionPages; p++) {
                uint64_t phys = regionStart + p * (uint64_t)PAGE_SIZE;

                if (phys < PHYS_BASE)
                    continue;
                if (phys >= PHYS_BASE + PHYS_SIZE)
                    break; /* rest of this descriptor is outside window */

                uint32_t idx = (uint32_t)((phys - PHYS_BASE) / PAGE_SIZE);
                /* idx < MAX_PAGES guaranteed by the range check above */
                bitmapClear(idx);
            }
        }
    }

    /* Step 5: re-mark kernel image pages as used */
    if (kernStart >= PHYS_BASE && kernEnd > kernStart) {
        uint32_t kFirst = (uint32_t)((kernStart - PHYS_BASE) / PAGE_SIZE);
        uint32_t kLast  = (uint32_t)((kernEnd - PHYS_BASE + PAGE_SIZE - 1u)
                                      / PAGE_SIZE);
        if (kLast > MAX_PAGES) kLast = MAX_PAGES;
        for (uint32_t i = kFirst; i < kLast; i++)
            bitmapSet(i);
    } else {
        for (uint32_t i = 0; i < KERNEL_RESERVED_PAGES_FALLBACK; i++)
            bitmapSet(i);
    }

findFirst:
    /* Step 6: find first free page */
    nextFreePage = MAX_PAGES; /* assume none */
    for (uint32_t i = 0; i < MAX_PAGES; i++) {
        if (!bitmapTest(i)) {
            nextFreePage = i;
            break;
        }
    }

    /* Step 7: build 64-bit page tables (static memory, no alloc needed) */
    buildPageTables();

    /* Step 8: report */
    uint32_t freeCount = 0;
    for (uint32_t i = 0; i < MAX_PAGES; i++)
        if (!bitmapTest(i)) freeCount++;

    printStr("MM: free pages: ", 0x0A);
    printNum(freeCount, 0x0A);
    printStr(" / ", 0x0A);
    printNum(MAX_PAGES, 0x0A);
    printStr("  first free: ", 0x0A);
    printNum(nextFreePage, 0x0A);
    printStr("\n", 0x0A);

    if (freeCount == 0)
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
void *mmAllocPage(void) {
    for (uint32_t i = nextFreePage; i < MAX_PAGES; i++) {
        if (!bitmapTest(i)) {
            bitmapSet(i);
            nextFreePage = i + 1u;
            return (void *)(uintptr_t)(PHYS_BASE + (uint64_t)i * PAGE_SIZE);
        }
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/* mmFreePage                                                          */
/* ------------------------------------------------------------------ */
void mmFreePage(void *page) {
    uint64_t addr = (uint64_t)(uintptr_t)page;
    if (addr < PHYS_BASE || addr >= PHYS_BASE + PHYS_SIZE)
        return;
    if (addr & (PAGE_SIZE - 1u))
        return; /* not page-aligned — refuse silently */
    uint32_t idx = (uint32_t)((addr - PHYS_BASE) / PAGE_SIZE);
    bitmapClear(idx);
    if (idx < nextFreePage)
        nextFreePage = idx;
}

/* ------------------------------------------------------------------ */
/* mmMapPage                                                           */
/* ------------------------------------------------------------------ */
int mmMapPage(uint32_t vaddr, uint32_t paddr, uint32_t flags) {
    if (!pageTablesReady)
        return -1;

    uint32_t pdIdx  = (vaddr >> 21) & 0x1FFu; /* PD index (2 MB granule) */
    uint32_t ptIdx  = (vaddr >> 12) & 0x1FFu; /* PT index */

    if (pdIdx >= NUM_PTS)
        return -1; /* outside identity-mapped range */

    uint64_t entry = (uint64_t)paddr | PTE_PRESENT;
    if (flags & MM_FLAG_WRITE) entry |= PTE_WRITABLE;
    if (flags & MM_FLAG_USER)  entry |= PTE_USER;

    pt[pdIdx][ptIdx] = entry;

    /* Invalidate TLB entry */
    __asm__ volatile("invlpg (%0)" : : "r"((uintptr_t)vaddr) : "memory");
    return 0;
}

/* ------------------------------------------------------------------ */
/* mmUnmapPage                                                         */
/* ------------------------------------------------------------------ */
int mmUnmapPage(uint32_t vaddr) {
    if (!pageTablesReady)
        return -1;
    uint32_t pdIdx = (vaddr >> 21) & 0x1FFu;
    uint32_t ptIdx = (vaddr >> 12) & 0x1FFu;
    if (pdIdx >= NUM_PTS)
        return -1;
    pt[pdIdx][ptIdx] = 0;
    __asm__ volatile("invlpg (%0)" : : "r"((uintptr_t)vaddr) : "memory");
    return 0;
}

/* ------------------------------------------------------------------ */
/* mmEnablePaging                                                      */
/* ------------------------------------------------------------------ */
/*
 * Precondition: buildPageTables() has been called (pageTablesReady == 1).
 * Panics if page tables are not ready — silent failure here would cause
 * undefined behaviour on the first memory access after return.
 */
void mmEnablePaging(void) {
    if (!pageTablesReady)
        mmPanic("mmEnablePaging called before page tables are built");

    uint64_t cr3 = (uint64_t)(uintptr_t)pml4;

    __asm__ volatile(
        "movq %0, %%cr3\n"
        "movq %%cr0, %%rax\n"
        "orl  $0x80000000, %%eax\n"
        "movq %%rax, %%cr0\n"
        :
        : "r"(cr3)
        : "rax", "memory"
    );
}
