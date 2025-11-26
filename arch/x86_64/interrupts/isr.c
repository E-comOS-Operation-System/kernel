/*
 * E-comOS - 中断服务例程C处理程序
 */

#include <stdint.h>

// 寄存器结构体
struct registers {
    uint32_t ds;                                     // 数据段选择子
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; // 通用寄存器
    uint32_t int_no, err_code;                       // 中断号和错误码
    uint32_t eip, cs, eflags, useresp, ss;          // 处理器推入的寄存器
};

// 异常消息数组
static const char* exception_messages[] = {
    "Division By Zero",
    "Debug",
    "Non Maskable Interrupt",
    "Breakpoint",
    "Into Detected Overflow",
    "Out of Bounds",
    "Invalid Opcode",
    "No Coprocessor",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Bad TSS",
    "Segment Not Present",
    "Stack Fault",
    "General Protection Fault",
    "Page Fault",
    "Unknown Interrupt",
    "Coprocessor Fault",
    "Alignment Check",
    "Machine Check",
    "SIMD Floating Point Exception",
    "Virtualization Exception",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved"
};

// 外部函数声明
extern void terminal_writestring(const char* data);

// 中断处理程序
void isr_handler(struct registers regs) {
    if (regs.int_no < 32) {
        // CPU异常
        terminal_writestring("Exception: ");
        terminal_writestring(exception_messages[regs.int_no]);
        terminal_writestring("\n");
        
        // 对于严重异常，停止系统
        if (regs.int_no == 8 || regs.int_no == 13 || regs.int_no == 14) {
            terminal_writestring("System halted due to critical exception\n");
            while (1) {
                __asm__ volatile ("cli; hlt");
            }
        }
    }
}