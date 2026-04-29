/*
    E-comOS Kernel - Debug + Early Init
    Copyright (C) 2025,2026  Saladin5101
*/

#include <kernel/debug.h>
#include <kernel/early_init.h>
#include <kernel/printkit/print.h>

/* Serial port COM1 for early debug output */
#define COM1 0x3F8

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

void early_debug_init(void) {
    outb(COM1 + 1, 0x00); /* disable interrupts */
    outb(COM1 + 3, 0x80); /* enable DLAB */
    outb(COM1 + 0, 0x03); /* 38400 baud low */
    outb(COM1 + 1, 0x00); /* 38400 baud high */
    outb(COM1 + 3, 0x03); /* 8 bits, no parity, 1 stop */
    outb(COM1 + 2, 0xC7); /* enable FIFO */
    outb(COM1 + 4, 0x0B); /* IRQs enabled, RTS/DSR set */
}

void early_debug_puts(const char *str) {
    for (; *str; str++) {
        while (!(inb(COM1 + 5) & 0x20))
            ;
        outb(COM1, (uint8_t)*str);
    }
}

void kernel_panic(const char *msg) {
    __asm__ volatile("cli");
    print_str("KERNEL PANIC: ", 0x4F);
    print_str(msg, 0x4F);
    print_str("\n", 0x4F);
    early_debug_puts("KERNEL PANIC: ");
    early_debug_puts(msg);
    early_debug_puts("\n");
    while (1)
        __asm__ volatile("hlt");
}

void kernel_log(const char *msg) {
    print_str("[LOG] ", 0x07);
    print_str(msg, 0x07);
    early_debug_puts("[LOG] ");
    early_debug_puts(msg);
}

int early_kernel_init(uint32_t multiboot_magic, uint32_t multiboot_info) {
    (void)multiboot_magic;
    (void)multiboot_info;
    early_debug_init();
    early_debug_puts("E-comOS early init OK\n");
    return 0;
}
