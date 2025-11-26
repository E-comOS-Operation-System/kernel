/*
 * E-comOS - 中断描述符表(IDT)管理
 */

#include <stdint.h>

#define IDT_ENTRIES 256

// IDT条目结构
struct idt_entry {
    uint16_t base_low;    // 中断处理程序地址低16位
    uint16_t selector;    // 内核段选择子
    uint8_t  always0;     // 保留，必须为0
    uint8_t  flags;       // 标志位
    uint16_t base_high;   // 中断处理程序地址高16位
} __attribute__((packed));

// IDT指针结构
struct idt_ptr {
    uint16_t limit;       // IDT大小
    uint32_t base;        // IDT基地址
} __attribute__((packed));

static struct idt_entry idt[IDT_ENTRIES];
static struct idt_ptr idt_pointer;

// 设置IDT条目
void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
    idt[num].base_low = base & 0xFFFF;
    idt[num].base_high = (base >> 16) & 0xFFFF;
    idt[num].selector = sel;
    idt[num].always0 = 0;
    idt[num].flags = flags;
}

// 初始化IDT
void idt_init(void) {
    idt_pointer.limit = sizeof(struct idt_entry) * IDT_ENTRIES - 1;
    idt_pointer.base = (uint32_t)&idt;
    
    // 清空IDT
    for (int i = 0; i < IDT_ENTRIES; i++) {
        idt_set_gate(i, 0, 0, 0);
    }
    
    // 加载IDT
    __asm__ volatile ("lidt %0" : : "m" (idt_pointer));
}