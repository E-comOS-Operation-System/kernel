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

#ifndef KERNEL_MICROKERNEL_H
#define KERNEL_MICROKERNEL_H

#include <stdint.h>

// ONLY 4 things in microkernel:

// 1. Address space management (privileged)
typedef uint32_t address_space_t;
address_space_t as_create(void);
int as_map_page(address_space_t as, uint32_t vaddr, uint32_t paddr, uint32_t flags);

// 2. Thread scheduling (minimal)
typedef uint32_t thread_t;
thread_t thread_create(address_space_t as, void *entry);
void thread_yield(void);

// 3. IPC primitives (cross-address-space)
struct ipc_msg {
    uint32_t sender;
    uint32_t size;
    uint8_t data[256];
};
int ipc_send(thread_t target, struct ipc_msg *msg);
int ipc_receive(struct ipc_msg *msg);

// 4. Interrupt handling (privileged)
int irq_wait(uint32_t irq_num);

#endif