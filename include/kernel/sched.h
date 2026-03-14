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

#ifndef KERNEL_SCHED_H
#define KERNEL_SCHED_H

#include <stdint.h>

typedef enum {
    THREAD_READY,
    THREAD_RUNNING,
    THREAD_BLOCKED,
    THREAD_TERMINATED
} thread_state_t;

struct thread {
    uint32_t id;
    thread_state_t state;
    uint32_t stack_ptr;
    uint32_t priority;
};

// Scheduler functions
int sched_create_thread(void (*entry_point)(void));
void sched_yield(void);
void sched_schedule(void);

#endif