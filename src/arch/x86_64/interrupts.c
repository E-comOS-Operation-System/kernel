/*
    E-comOS Kernel - A Microkernel for E-comOS
    Copyright (C) 2025  Saladin5101

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

#include <kernel/arch/interrupts.h>
#include <kernel/arch/x86_64.h>

static struct idt_entry idt[256];
static struct idt_ptr idt_pointer;

void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
    idt[num].offset_low = base & 0xFFFF;
    idt[num].offset_high = (base >> 16) & 0xFFFF;
    idt[num].selector = sel;
    idt[num].zero = 0;
    idt[num].type_attr = flags;
}

void idt_init(void) {
    idt_pointer.limit = sizeof(idt) - 1;
    idt_pointer.base = (uint32_t)&idt;
    
    // Load IDT
    __asm__ volatile ("lidt %0" : : "m" (idt_pointer));
}

void irq_remap(void) {
    // Remap PIC IRQs to avoid conflicts with CPU exceptions
    // Master PIC: IRQ 0-7 -> INT 32-39
    // Slave PIC: IRQ 8-15 -> INT 40-47
    
    // Start initialization sequence
    __asm__ volatile ("outb %0, %1" : : "a"((uint8_t)0x11), "Nd"((uint16_t)0x20));
    __asm__ volatile ("outb %0, %1" : : "a"((uint8_t)0x11), "Nd"((uint16_t)0xA0));
    
    // Set vector offsets
    __asm__ volatile ("outb %0, %1" : : "a"((uint8_t)0x20), "Nd"((uint16_t)0x21));
    __asm__ volatile ("outb %0, %1" : : "a"((uint8_t)0x28), "Nd"((uint16_t)0xA1));
    
    // Configure cascade
    __asm__ volatile ("outb %0, %1" : : "a"((uint8_t)0x04), "Nd"((uint16_t)0x21));
    __asm__ volatile ("outb %0, %1" : : "a"((uint8_t)0x02), "Nd"((uint16_t)0xA1));
    
    // Set 8086 mode
    __asm__ volatile ("outb %0, %1" : : "a"((uint8_t)0x01), "Nd"((uint16_t)0x21));
    __asm__ volatile ("outb %0, %1" : : "a"((uint8_t)0x01), "Nd"((uint16_t)0xA1));
    
    // Mask all IRQs initially
    __asm__ volatile ("outb %0, %1" : : "a"((uint8_t)0xFF), "Nd"((uint16_t)0x21));
    __asm__ volatile ("outb %0, %1" : : "a"((uint8_t)0xFF), "Nd"((uint16_t)0xA1));
}

// Stub ISR handlers
void isr0(void) { while(1); }
void isr13(void) { while(1); }
void isr14(void) { while(1); }
void irq0(void) { while(1); }
void irq1(void) { while(1); }

void arch_enable_interrupts(void) {
    __asm__ volatile ("sti");
}

void arch_disable_interrupts(void) {
    __asm__ volatile ("cli");
}

void arch_halt(void) {
    __asm__ volatile ("hlt");
}