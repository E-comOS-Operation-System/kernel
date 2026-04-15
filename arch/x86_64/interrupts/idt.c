/*
    E-comOS Kernel - Interrupt Descriptor Table (64-bit)
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

#include <stdint.h>
#include <kernel/arch/interrupts.h>

#define IDT_ENTRIES 256

typedef struct {
    uint16_t offsetLow;
    uint16_t selector;
    uint8_t  ist;
    uint8_t  flags;
    uint16_t offsetMid;
    uint32_t offsetHigh;
    uint32_t reserved;
} __attribute__((packed)) IdtEntry;

typedef struct {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed)) IdtPtr;

static IdtEntry idt[IDT_ENTRIES];
static IdtPtr   idtPointer;

/* Public stub kept for ABI compatibility — real work done by setGate64 */
void idtSetGate(uint8_t num, uint64_t base, uint16_t sel, uint8_t flags) {
    idt[num].offsetLow  = base & 0xFFFF;
    idt[num].selector   = sel;
    idt[num].ist        = 0;
    idt[num].flags      = flags;
    idt[num].offsetMid  = (base >> 16) & 0xFFFF;
    idt[num].offsetHigh = (base >> 32) & 0xFFFFFFFF;
    idt[num].reserved   = 0;
}

#define GATE(n, fn)  idtSetGate((n), (uint64_t)(uintptr_t)(fn), 0x08, 0x8E)
#define UGATE(n, fn) idtSetGate((n), (uint64_t)(uintptr_t)(fn), 0x08, 0xEE)

void idtInit(void) {
    for (int i = 0; i < IDT_ENTRIES; i++) {
        idt[i].offsetLow  = 0;
        idt[i].selector   = 0;
        idt[i].ist        = 0;
        idt[i].flags      = 0;
        idt[i].offsetMid  = 0;
        idt[i].offsetHigh = 0;
        idt[i].reserved   = 0;
    }

    GATE(0,  isr0);  GATE(1,  isr1);  GATE(2,  isr2);  GATE(3,  isr3);
    GATE(4,  isr4);  GATE(5,  isr5);  GATE(6,  isr6);  GATE(7,  isr7);
    GATE(8,  isr8);  GATE(9,  isr9);  GATE(10, isr10); GATE(11, isr11);
    GATE(12, isr12); GATE(13, isr13); GATE(14, isr14); GATE(15, isr15);
    GATE(16, isr16); GATE(17, isr17); GATE(18, isr18); GATE(19, isr19);
    GATE(20, isr20); GATE(21, isr21); GATE(22, isr22); GATE(23, isr23);
    GATE(24, isr24); GATE(25, isr25); GATE(26, isr26); GATE(27, isr27);
    GATE(28, isr28); GATE(29, isr29); GATE(30, isr30); GATE(31, isr31);

    GATE(32, irq0);  GATE(33, irq1);  GATE(34, irq2);  GATE(35, irq3);
    GATE(36, irq4);  GATE(37, irq5);  GATE(38, irq6);  GATE(39, irq7);
    GATE(40, irq8);  GATE(41, irq9);  GATE(42, irq10); GATE(43, irq11);
    GATE(44, irq12); GATE(45, irq13); GATE(46, irq14); GATE(47, irq15);

    UGATE(128, isr128);

    idtPointer.limit = sizeof(idt) - 1;
    idtPointer.base  = (uint64_t)(uintptr_t)&idt;
    __asm__ volatile("lidt %0" : : "m"(idtPointer));
}
