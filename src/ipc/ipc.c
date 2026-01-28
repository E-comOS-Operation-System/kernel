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

#include <kernel/ipc.h>

#define MAX_IPC_QUEUE 32

static struct ipc_message ipc_queue[MAX_IPC_QUEUE];
static uint32_t queue_head = 0;
static uint32_t queue_tail = 0;

int ipc_send(thread_id_t target, struct ipc_message *msg) {
    uint32_t next_tail = (queue_tail + 1) % MAX_IPC_QUEUE;
    if (next_tail == queue_head) {
        return -1; // Queue full
    }
    
    ipc_queue[queue_tail] = *msg;
    ipc_queue[queue_tail].sender = target; // Set sender ID
    queue_tail = next_tail;
    return 0;
}

int ipc_receive(struct ipc_message *msg) {
    if (queue_head == queue_tail) {
        return -1; // No messages
    }
    
    *msg = ipc_queue[queue_head];
    queue_head = (queue_head + 1) % MAX_IPC_QUEUE;
    return 0;
}