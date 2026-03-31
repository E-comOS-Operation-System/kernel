/*
 * E-comOS - 硬件中断请求(IRQ)处理程序 (64-bit)
 */

#include <stdint.h>
#include <kernel/time.h>

#define PIC1_COMMAND 0x20
#define PIC1_DATA    0x21
#define PIC2_COMMAND 0xA0
#define PIC2_DATA    0xA1

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

void irq_remap(void) {
    uint8_t a1 = inb(PIC1_DATA);
    uint8_t a2 = inb(PIC2_DATA);

    outb(PIC1_COMMAND, 0x11);
    outb(PIC2_COMMAND, 0x11);

    outb(PIC1_DATA, 0x20);
    outb(PIC2_DATA, 0x28);

    outb(PIC1_DATA, 0x04);
    outb(PIC2_DATA, 0x02);

    outb(PIC1_DATA, 0x01);
    outb(PIC2_DATA, 0x01);

    outb(PIC1_DATA, a1);
    outb(PIC2_DATA, a2);
}

void irq_ack(uint8_t irq) {
    if (irq >= 8)
        outb(PIC2_COMMAND, 0x20);
    outb(PIC1_COMMAND, 0x20);
}

static void (*irq_handlers[16])(void) = {0};

void irq_install_handler(uint8_t irq, void (*handler)(void)) {
    if (irq < 16)
        irq_handlers[irq] = handler;
}

void irq_uninstall_handler(uint8_t irq) {
    if (irq < 16)
        irq_handlers[irq] = 0;
}

static void timer_handler(void) {
    time_tick();
}

/* Called from irq.s with vector number in rdi */
void irq_handler_asm_shim(uint64_t vec) {
    uint8_t irq = (uint8_t)(vec - 32);
    if (irq < 16 && irq_handlers[irq])
        irq_handlers[irq]();
    irq_ack(irq);
}

void irq_init_timer(void) {
    irq_install_handler(0, timer_handler);
}
