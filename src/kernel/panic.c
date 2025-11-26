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

#include <kernel/debug.h>

void kernel_panic(const char *message) {
    (void)message; // Suppress unused parameter warning
    
    // Disable interrupts
    __asm__ volatile ("cli");
    
    // TODO: Display panic message
    // For now, just halt
    while (1) {
        __asm__ volatile ("hlt");
    }
}

void kernel_log(const char *message) {
    (void)message; // Suppress unused parameter warning
    // TODO: Implement kernel logging
}