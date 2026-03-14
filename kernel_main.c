/*
    E-comOS Kernel - A Microkernel for E-comOS
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

// Forward declarations
extern void kernel_main(void);
extern void init_service_entry(void);

// Assembly entry point that calls our C entry
__asm__(
    ".section .text\n"
    ".global _start\n"
    "_start:\n"
    "    movq $0x200000, %rsp\n"  // Set stack pointer
    "    cld\n"                    // Clear direction flag
    "    call kernel_entry\n"     // Call C entry point
    "    cli\n"                    // Disable interrupts
    "1:  hlt\n"                   // Halt
    "    jmp 1b\n"                // Loop forever
);

// C kernel entry point
void kernel_entry(void) {
    // Set up stack and call main kernel
    __asm__ volatile (
        "mov $0x200000, %%rsp\n"
        "mov %%rsp, %%rbp\n"
        :
        :
        : "memory"
    );
    
    kernel_main();
}