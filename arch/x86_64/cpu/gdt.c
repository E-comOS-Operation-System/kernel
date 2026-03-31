/*
 * E-comOS - Global Descriptor Table + TSS
 */

#include <stdint.h>

struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_middle;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high;
} __attribute__((packed));

struct gdt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

/*
 * TSS — only the fields the CPU actually reads on ring-3 -> ring-0 transition.
 * esp0 / ss0 tell the CPU where to put the kernel stack when an interrupt or
 * syscall arrives from user mode.
 */
struct tss_entry {
    uint32_t prev_tss;
    uint32_t esp0;       /* kernel stack pointer loaded on ring-3 -> ring-0 */
    uint32_t ss0;        /* kernel stack segment (0x10)                      */
    uint32_t esp1, ss1;
    uint32_t esp2, ss2;
    uint32_t cr3, eip, eflags;
    uint32_t eax, ecx, edx, ebx, esp, ebp, esi, edi;
    uint32_t es, cs, ss, ds, fs, gs;
    uint32_t ldt;
    uint16_t trap, iomap_base;
} __attribute__((packed));

/*
 * Index 0: null
 * Index 1: kernel code  (ring 0)  selector 0x08
 * Index 2: kernel data  (ring 0)  selector 0x10
 * Index 3: user   code  (ring 3)  selector 0x1B  (0x18 | RPL 3)
 * Index 4: user   data  (ring 3)  selector 0x23  (0x20 | RPL 3)
 * Index 5: TSS                    selector 0x28
 */
static struct gdt_entry gdt[6];
static struct gdt_ptr   gdtp;
static struct tss_entry tss;

/* 4 KB kernel stack used when returning from user mode via int 0x80 / IRQ */
static uint8_t kernel_stack[4096] __attribute__((aligned(16)));

static void gdt_set(int i, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt[i].base_low    = base & 0xFFFF;
    gdt[i].base_middle = (base >> 16) & 0xFF;
    gdt[i].base_high   = (base >> 24) & 0xFF;
    gdt[i].limit_low   = limit & 0xFFFF;
    gdt[i].granularity = ((limit >> 16) & 0x0F) | (gran & 0xF0);
    gdt[i].access      = access;
}

static void tss_init(void) {
    uint32_t base  = (uint32_t)(uintptr_t)&tss;
    uint32_t limit = sizeof(tss) - 1;

    /* TSS descriptor: present, ring 0, 32-bit available TSS (type 0x89) */
    gdt_set(5, base, limit, 0x89, 0x00);

    tss.ss0  = 0x10;                                        /* kernel data seg */
    tss.esp0 = (uint32_t)(uintptr_t)(kernel_stack + 4096); /* top of stack    */
    tss.iomap_base = sizeof(tss);                           /* no I/O bitmap   */

    __asm__ volatile ("ltr %%ax" : : "a"((uint16_t)0x28));
}

void gdt_init(void) {
    gdt_set(0, 0, 0,          0x00, 0x00); /* null                        */
    gdt_set(1, 0, 0xFFFFFFFF, 0x9A, 0xCF); /* kernel code ring 0          */
    gdt_set(2, 0, 0xFFFFFFFF, 0x92, 0xCF); /* kernel data ring 0          */
    gdt_set(3, 0, 0xFFFFFFFF, 0xFA, 0xCF); /* user   code ring 3 (DPL=3)  */
    gdt_set(4, 0, 0xFFFFFFFF, 0xF2, 0xCF); /* user   data ring 3 (DPL=3)  */
    gdt_set(5, 0, 0,          0x00, 0x00); /* TSS placeholder, set below  */

    gdtp.limit = sizeof(gdt) - 1;
    gdtp.base  = (uint32_t)(uintptr_t)&gdt;

    __asm__ volatile (
        "lgdt %0\n"
        /* Reload CS in 64-bit mode: push cs:rip onto stack then lretq */
        "pushq $0x08\n"
        "leaq 1f(%%rip), %%rax\n"
        "pushq %%rax\n"
        "lretq\n"
        "1:\n"
        "movw $0x10, %%ax\n"
        "movw %%ax,  %%ds\n"
        "movw %%ax,  %%es\n"
        "movw %%ax,  %%ss\n"
        "movw %%ax,  %%fs\n"
        "movw %%ax,  %%gs\n"
        : : "m"(gdtp) : "rax", "memory"
    );

    tss_init();
}

/* Update kernel stack pointer in TSS (call after creating a new kernel thread) */
void tss_set_kernel_stack(uint32_t esp0) {
    tss.esp0 = esp0;
}