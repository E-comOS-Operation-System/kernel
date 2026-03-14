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
#define IPC_MAX_DATA_SIZE    4096
#include <stdint.h>

typedef uint32_t thread_id_t;

struct IPCMessage {
  uint32_t type;
  uint32_t source;
  uint32_t target;
  uint32_t timestamp;
  uint32_t size;
  uint32_t sequence;
  uint8_t data[IPC_MAX_DATA_SIZE];
};

int ipc_send(thread_id_t target, struct ipc_message *msg);
int ipc_receive(struct ipc_message *msg);

#endif
