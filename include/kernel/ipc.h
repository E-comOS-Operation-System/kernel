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

#ifndef KERNEL_IPC_H
#define KERNEL_IPC_H

#include <stdint.h>

typedef uint32_t thread_id_t;

struct ipc_message {
    thread_id_t sender;
    uint32_t size;
    uint8_t data[256];
};

int ipc_send(thread_id_t target, struct ipc_message *msg);
int ipc_receive(struct ipc_message *msg);

#endif