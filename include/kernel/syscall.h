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

#ifndef KERNEL_SYSCALL_H
#define KERNEL_SYSCALL_H

#include <stdint.h>

// E-comOS Microkernel - MINIMAL system calls
// Only hardware-privileged operations in kernel

#define SYS_IPC_SEND        1  // Cross-address-space message passing
#define SYS_IPC_RECEIVE     2  // Receive message (blocking)
#define SYS_THREAD_YIELD    3  // Yield CPU to scheduler
#define SYS_ADDRESS_MAP     4  // Map physical page (privileged)
#define SYS_IRQ_WAIT        5  // Wait for hardware interrupt
#define SYS_IRQ_GET_COUNT   6  // Get IRQ occurrence count
#define SYS_IRQ_RESET_COUNT 7  // Reset IRQ occurrence count

// Everything else (objects, services, processes) 
// implemented as USERSPACE services!

// Process states
#define PROCESS_STATE_BLOCKED  1
#define PROCESS_STATE_READY    2

// Block reasons
#define BLOCK_REASON_NONE      0
#define BLOCK_REASON_IRQ_WAIT  1

// IRQ wait flags
#define IRQ_WAIT_CLEAR         0x01
#define IRQ_WAIT_NOWAIT        0x02

// Error codes
#define ERR_TIMEOUT            -3

// System call handler
long syscall_handler(uint32_t syscall_num, uint32_t arg1, uint32_t arg2, uint32_t arg3);

#endif