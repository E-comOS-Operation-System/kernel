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

void earlyDebugInit(void) {
    outb(COM1 + 1, 0x00); /* disable interrupts */
    outb(COM1 + 3, 0x80); /* enable DLAB */
    outb(COM1 + 0, 0x03); /* 38400 baud low */
    outb(COM1 + 1, 0x00); /* 38400 baud high */
    outb(COM1 + 3, 0x03); /* 8 bits, no parity, 1 stop */
    outb(COM1 + 2, 0xC7); /* enable FIFO */
    outb(COM1 + 4, 0x0B); /* IRQs enabled, RTS/DSR set */
}

void earlyDebugPuts(const char *str) {
    for (; *str; str++) {
        while (!(inb(COM1 + 5) & 0x20))
            ;
        outb(COM1, (uint8_t)*str);
    }
}

void kernelPanic(const char *msg) {
    __asm__ volatile("cli");
    printStr("KERNEL PANIC: ", 0x4F);
    printStr(msg, 0x4F);
    printStr("\n", 0x4F);
    earlyDebugPuts("KERNEL PANIC: ");
    earlyDebugPuts(msg);
    earlyDebugPuts("\n");
    while (1)
        __asm__ volatile("hlt");
}

void kernelLog(const char *msg) {
    printStr("[LOG] ", 0x07);
    printStr(msg, 0x07);
    earlyDebugPuts("[LOG] ");
    earlyDebugPuts(msg);
}

int earlyKernelInit(uint32_t multibootMagic, uint32_t multibootInfo) {
    (void)multibootMagic;
    (void)multibootInfo;
    earlyDebugInit();
    earlyDebugPuts("E-comOS early init OK\n");
    return 0;
}
