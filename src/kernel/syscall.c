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

#include <kernel/syscall.h>
#include <kernel/ipc.h>
#include <kernel/sched.h>
#include <kernel/mm.h>

long syscall_handler(uint32_t syscall_num, uint32_t arg1, uint32_t arg2, uint32_t arg3) {
    switch (syscall_num) {
        case SYS_IPC_SEND:
            return ipc_send((thread_id_t)arg1, (struct ipc_message*)arg2);
            
        case SYS_IPC_RECEIVE:
            return ipc_receive((struct ipc_message*)arg1);
            
        case SYS_THREAD_YIELD:
            sched_yield();
            return 0;
            
        case SYS_ADDRESS_MAP:
            return mm_map_page(arg1, arg2, arg3);
            
        case SYS_IRQ_WAIT:
            // TODO: Implement IRQ waiting
            return 0;
            
        default:
            return -1; // Invalid syscall
    }
}