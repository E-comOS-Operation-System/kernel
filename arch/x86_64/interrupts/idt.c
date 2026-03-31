/*
 * E-comOS - 中断描述符表(IDT)管理 (64-bit)
 */

#include <stdint.h>
#include <kernel/arch/interrupts.h>

#define IDT_ENTRIES 256

/* 64-bit IDT gate descriptor */
struct idt_entry64 {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t  ist;        /* interrupt stack table (0 = none) */
    uint8_t  flags;
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t reserved;
} __attribute__((packed));

struct idt_ptr64 {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

static struct idt_entry64 idt[IDT_ENTRIES];
static struct idt_ptr64   idt_pointer;

void idt_set_gate(uint8_t num, uint32_t base32, uint16_t sel, uint8_t flags) {
    /* base32 is kept for ABI compat; promote to 64-bit via the extern symbol */
    (void)base32; (void)sel; (void)flags;
    /* Real work done by idt_set_gate64 below; this stub satisfies old callers */
}

static void idt_set_gate64(uint8_t num, uint64_t base, uint16_t sel,
                            uint8_t flags) {
    idt[num].offset_low  = base & 0xFFFF;
    idt[num].selector    = sel;
    idt[num].ist         = 0;
    idt[num].flags       = flags;
    idt[num].offset_mid  = (base >> 16) & 0xFFFF;
    idt[num].offset_high = (base >> 32) & 0xFFFFFFFF;
    idt[num].reserved    = 0;
}

#define GATE(n, fn)  idt_set_gate64((n), (uint64_t)(uintptr_t)(fn), 0x08, 0x8E)
#define UGATE(n, fn) idt_set_gate64((n), (uint64_t)(uintptr_t)(fn), 0x08, 0xEE)

void idt_init(void) {
    for (int i = 0; i < IDT_ENTRIES; i++) {
        idt[i].offset_low  = 0;
        idt[i].selector    = 0;
        idt[i].ist         = 0;
        idt[i].flags       = 0;
        idt[i].offset_mid  = 0;
        idt[i].offset_high = 0;
        idt[i].reserved    = 0;
    }

    /* CPU exceptions */
    GATE(0,  isr0);  GATE(1,  isr1);  GATE(2,  isr2);  GATE(3,  isr3);
    GATE(4,  isr4);  GATE(5,  isr5);  GATE(6,  isr6);  GATE(7,  isr7);
    GATE(8,  isr8);  GATE(9,  isr9);  GATE(10, isr10); GATE(11, isr11);
    GATE(12, isr12); GATE(13, isr13); GATE(14, isr14); GATE(15, isr15);
    GATE(16, isr16); GATE(17, isr17); GATE(18, isr18); GATE(19, isr19);
    GATE(20, isr20); GATE(21, isr21); GATE(22, isr22); GATE(23, isr23);
    GATE(24, isr24); GATE(25, isr25); GATE(26, isr26); GATE(27, isr27);
    GATE(28, isr28); GATE(29, isr29); GATE(30, isr30); GATE(31, isr31);

    /* Hardware IRQs */
    GATE(32, irq0);  GATE(33, irq1);  GATE(34, irq2);  GATE(35, irq3);
    GATE(36, irq4);  GATE(37, irq5);  GATE(38, irq6);  GATE(39, irq7);
    GATE(40, irq8);  GATE(41, irq9);  GATE(42, irq10); GATE(43, irq11);
    GATE(44, irq12); GATE(45, irq13); GATE(46, irq14); GATE(47, irq15);

    /* int 0x80 syscall — DPL=3 so ring-3 code can call it */
    UGATE(128, isr128);

    idt_pointer.limit = sizeof(idt) - 1;
    idt_pointer.base  = (uint64_t)(uintptr_t)&idt;

    __asm__ volatile ("lidt %0" : : "m"(idt_pointer));
}
