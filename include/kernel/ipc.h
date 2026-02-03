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

#ifndef KERNEL_IPC_H
#define KERNEL_IPC_H

#include <stdint.h>

// Return codes to match C library
#define ECLIB_OK 0
#define ECLIB_IPC_TIMEOUT -1
#define ECLIB_IPC_SERVICE_UNAVAIL -2
#define ECLIB_IPC_PERMISSION_DENIED -3
#define ECLIB_IPC_BUFFER_OVERFLOW -4

typedef uint32_t thread_id_t;

// IPC message structure to match C library
typedef struct {
    uint32_t type;
    uint32_t flags;
    thread_id_t sender_pid;
    thread_id_t receiver_pid;
    uint32_t data_len;
    uint8_t payload[256];
} ipc_message_t;

// Kernel IPC functions
int ipc_send(thread_id_t target, ipc_message_t *msg);
int ipc_receive(ipc_message_t *msg);
int ipc_receive_msg(ipc_message_t* msg, int timeout_ms);
int ipc_send_msg(uint32_t type, uint32_t flags, uint32_t receiver_pid, 
                 uint32_t data_len, const void* data);

#endif