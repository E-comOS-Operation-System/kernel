/*
    E-com_os Kernel - ISR C handler
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
#include <kernel/printkit/print.h>

static const char *exception_messages[] = {
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

void isr_handler(uint64_t int_no, uint64_t err_code) {
    (void)err_code;
    if (int_no < 32) {
        print_str("Exception: ", 0x4F);
        print_str(exception_messages[int_no], 0x4F);
        print_str("\n", 0x4F);
        if (int_no == 8 || int_no == 13 || int_no == 14) {
            print_str("System halted\n", 0x4F);
            while (1)
                __asm__ volatile("cli; hlt");
        }
    }
}
