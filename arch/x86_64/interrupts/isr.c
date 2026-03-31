/*
 * E-comOS - 中断服务例程C处理程序
 */

#include <stdint.h>
#include <kernel/printkit/print.h>

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
    "Reserved", "Reserved", "Reserved", "Reserved",
    "Reserved", "Reserved", "Reserved", "Reserved",
    "Reserved", "Reserved", "Reserved"
};

/* Called from isr.s with (int_no, err_code) in rdi, rsi */
void isr_handler(uint64_t int_no, uint64_t err_code) {
    (void)err_code;
    if (int_no < 32) {
        print("Exception: ", 0x4F);
        print(exception_messages[int_no], 0x4F);
        print("\n", 0x4F);
        if (int_no == 8 || int_no == 13 || int_no == 14) {
            print("System halted\n", 0x4F);
            while (1) __asm__ volatile ("cli; hlt");
        }
    }
}