/*
    E-comOS Kernel - Global Descriptor Table + TSS (64-bit)
    Copyright (C) 2025,2026  Saladin5101

    GDT layout:
      0x00  null
      0x08  kernel code  (ring 0, 64-bit)
      0x10  kernel data  (ring 0)
      0x18  user   code  (ring 3, 64-bit)   selector 0x1B (|3)
      0x20  user   data  (ring 3)            selector 0x23 (|3)
      0x28  TSS low  (16 bytes, two GDT slots)
      0x30  TSS high

    64-bit TSS (Intel SDM Vol.3 §7.7):
      rsp0 at offset +4 (used on ring-3 → ring-0 transition)
*/

#include <stdint.h>

/* ------------------------------------------------------------------ */
/* GDT entry (8 bytes)                                                */
/* ------------------------------------------------------------------ */
typedef struct {
    uint16_t limitLow;
    uint16_t baseLow;
    uint8_t  baseMiddle;
    uint8_t  access;
    uint8_t  granularity;
    uint8_t  baseHigh;
} __attribute__((packed)) GdtEntry;

/* ------------------------------------------------------------------ */
/* GDTR (10 bytes for 64-bit mode)                                    */
/* ------------------------------------------------------------------ */
typedef struct {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed)) GdtPtr64;

/* ------------------------------------------------------------------ */
/* 64-bit TSS descriptor (16 bytes = two GDT slots)                  */
/* ------------------------------------------------------------------ */
typedef struct {
    uint16_t limitLow;
    uint16_t baseLow;
    uint8_t  baseMiddle;
    uint8_t  access;      /* 0x89 = present, ring-0, available 64-bit TSS */
    uint8_t  granularity;
    uint8_t  baseHigh;
    uint32_t baseUpper;
    uint32_t reserved;
} __attribute__((packed)) TssDescriptor;

/* ------------------------------------------------------------------ */
/* 64-bit TSS body (Intel SDM Vol.3 §7.7, Table 7-11)               */
/* ------------------------------------------------------------------ */
typedef struct {
    uint32_t reserved0;
    uint64_t rsp0;        /* kernel stack for ring-3 → ring-0            */
    uint64_t rsp1;
    uint64_t rsp2;
    uint64_t reserved1;
    uint64_t ist[7];      /* interrupt stack table (IST1..IST7)          */
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iomapBase;   /* offset to I/O permission bitmap             */
} __attribute__((packed)) Tss64;

/* ------------------------------------------------------------------ */
/* Static storage                                                     */
/* ------------------------------------------------------------------ */
/* 5 normal entries + 2 slots for the 64-bit TSS descriptor */
static GdtEntry    gdt[5];
static TssDescriptor tssDesc;
static GdtPtr64    gdtp;
static Tss64       tss;

static uint8_t kernelStack[4096] __attribute__((aligned(16)));

/* ------------------------------------------------------------------ */
/* Helpers                                                            */
/* ------------------------------------------------------------------ */
static void gdtSet(int i, uint32_t base, uint32_t limit,
                   uint8_t access, uint8_t gran) {
    gdt[i].baseLow     = (uint16_t)(base & 0xFFFFu);
    gdt[i].baseMiddle  = (uint8_t)((base >> 16) & 0xFFu);
    gdt[i].baseHigh    = (uint8_t)((base >> 24) & 0xFFu);
    gdt[i].limitLow    = (uint16_t)(limit & 0xFFFFu);
    gdt[i].granularity = (uint8_t)(((limit >> 16) & 0x0Fu) | (gran & 0xF0u));
    gdt[i].access      = access;
}

static void tssDescSet(uint64_t base, uint32_t limit) {
    tssDesc.limitLow   = (uint16_t)(limit & 0xFFFFu);
    tssDesc.baseLow    = (uint16_t)(base & 0xFFFFu);
    tssDesc.baseMiddle = (uint8_t)((base >> 16) & 0xFFu);
    tssDesc.access     = 0x89u; /* present, DPL=0, available 64-bit TSS */
    tssDesc.granularity = (uint8_t)(((limit >> 16) & 0x0Fu));
    tssDesc.baseHigh   = (uint8_t)((base >> 24) & 0xFFu);
    tssDesc.baseUpper  = (uint32_t)(base >> 32);
    tssDesc.reserved   = 0;
}

/* ------------------------------------------------------------------ */
/* gdtInit                                                            */
/* ------------------------------------------------------------------ */
/*
 * Precondition:  called before any ring-3 code or interrupt.
 * Postcondition: GDT loaded, TSS loaded, segment registers updated.
 */
void gdtInit(void) {
    /* Null descriptor */
    gdtSet(0, 0, 0, 0x00u, 0x00u);
    /* Kernel code: 64-bit, ring 0 (L=1 in granularity byte) */
    gdtSet(1, 0, 0xFFFFFu, 0x9Au, 0xA0u); /* 0xA0 = G=1, L=1 (64-bit) */
    /* Kernel data: ring 0 */
    gdtSet(2, 0, 0xFFFFFu, 0x92u, 0xC0u);
    /* User code: 64-bit, ring 3 */
    gdtSet(3, 0, 0xFFFFFu, 0xFAu, 0xA0u);
    /* User data: ring 3 */
    gdtSet(4, 0, 0xFFFFFu, 0xF2u, 0xC0u);

    /* Build a flat GDT: [gdt entries][tss descriptor] */
    gdtp.limit = (uint16_t)(sizeof(gdt) + sizeof(tssDesc) - 1u);
    gdtp.base  = (uint64_t)(uintptr_t)gdt;

    /* TSS */
    tss.rsp0      = (uint64_t)(uintptr_t)(kernelStack + sizeof(kernelStack));
    tss.iomapBase = (uint16_t)sizeof(Tss64); /* no I/O bitmap */

    tssDescSet((uint64_t)(uintptr_t)&tss, (uint32_t)(sizeof(Tss64) - 1u));

    /* We need the TSS descriptor contiguous with gdt[] in memory.
     * Since C doesn't guarantee struct layout across separate arrays,
     * we embed the TSS descriptor address directly in the GDTR. */
    gdtp.limit = (uint16_t)(sizeof(gdt) + sizeof(tssDesc) - 1u);
    /* GDTR base points to gdt[0]; TSS descriptor follows immediately
     * only if they are adjacent.  Use a packed struct trick: */

    /* Simpler: build a single flat table in a local array */
    static uint8_t gdtFlat[sizeof(gdt) + sizeof(tssDesc)]
        __attribute__((aligned(8)));

    /* Copy normal entries */
    for (uint32_t i = 0; i < sizeof(gdt); i++)
        gdtFlat[i] = ((uint8_t *)gdt)[i];
    /* Copy TSS descriptor */
    for (uint32_t i = 0; i < sizeof(tssDesc); i++)
        gdtFlat[sizeof(gdt) + i] = ((uint8_t *)&tssDesc)[i];

    gdtp.limit = (uint16_t)(sizeof(gdtFlat) - 1u);
    gdtp.base  = (uint64_t)(uintptr_t)gdtFlat;

    __asm__ volatile(
        "lgdt %0\n"
        /* Far return to reload CS with kernel code selector 0x08 */
        "pushq $0x08\n"
        "leaq  1f(%%rip), %%rax\n"
        "pushq %%rax\n"
        "lretq\n"
        "1:\n"
        "movw $0x10, %%ax\n"   /* kernel data selector */
        "movw %%ax, %%ds\n"
        "movw %%ax, %%es\n"
        "movw %%ax, %%ss\n"
        "xorw %%ax, %%ax\n"    /* FS/GS = null in 64-bit mode */
        "movw %%ax, %%fs\n"
        "movw %%ax, %%gs\n"
        : : "m"(gdtp) : "rax", "memory"
    );

    /* TSS selector = offset of tssDesc in gdtFlat = sizeof(gdt) = 0x28
     * RPL = 0, TI = 0  →  selector = 0x28 */
    __asm__ volatile("ltr %%ax" : : "a"((uint16_t)0x28u));
}

/* Update kernel stack pointer in TSS (call on each context switch) */
void tssSetKernelStack(uint64_t rsp0) {
    tss.rsp0 = rsp0;
}
