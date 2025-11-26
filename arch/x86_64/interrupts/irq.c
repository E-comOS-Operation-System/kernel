/*
 * E-comOS - 硬件中断请求(IRQ)处理程序
 */

#include <stdint.h>

// 寄存器结构体
struct registers {
    uint32_t ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t int_no, err_code;
    uint32_t eip, cs, eflags, useresp, ss;
};

// PIC端口定义
#define PIC1_COMMAND 0x20
#define PIC1_DATA    0x21
#define PIC2_COMMAND 0xA0
#define PIC2_DATA    0xA1

// 端口I/O函数
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// 重新映射PIC
void irq_remap(void) {
    // 保存当前屏蔽字
    uint8_t a1 = inb(PIC1_DATA);
    uint8_t a2 = inb(PIC2_DATA);
    
    // 初始化PIC
    outb(PIC1_COMMAND, 0x11);  // 初始化命令
    outb(PIC2_COMMAND, 0x11);
    
    // 设置中断向量偏移
    outb(PIC1_DATA, 0x20);     // IRQ 0-7 映射到 32-39
    outb(PIC2_DATA, 0x28);     // IRQ 8-15 映射到 40-47
    
    // 设置级联
    outb(PIC1_DATA, 0x04);     // 告诉主PIC从PIC在IRQ2
    outb(PIC2_DATA, 0x02);     // 告诉从PIC它的级联标识
    
    // 设置模式
    outb(PIC1_DATA, 0x01);     // 8086模式
    outb(PIC2_DATA, 0x01);
    
    // 恢复屏蔽字
    outb(PIC1_DATA, a1);
    outb(PIC2_DATA, a2);
}

// 发送EOI信号
void irq_ack(uint8_t irq) {
    if (irq >= 8) {
        outb(PIC2_COMMAND, 0x20);  // 发送EOI到从PIC
    }
    outb(PIC1_COMMAND, 0x20);      // 发送EOI到主PIC
}

// IRQ处理程序数组
static void (*irq_handlers[16])(struct registers*) = {0};

// 安装IRQ处理程序
void irq_install_handler(uint8_t irq, void (*handler)(struct registers*)) {
    if (irq < 16) {
        irq_handlers[irq] = handler;
    }
}

// 卸载IRQ处理程序
void irq_uninstall_handler(uint8_t irq) {
    if (irq < 16) {
        irq_handlers[irq] = 0;
    }
}

// IRQ处理程序
void irq_handler(struct registers regs) {
    uint8_t irq = regs.int_no - 32;
    
    // 调用已安装的处理程序
    if (irq_handlers[irq]) {
        irq_handlers[irq](&regs);
    }
    
    // 发送EOI信号
    irq_ack(irq);
}