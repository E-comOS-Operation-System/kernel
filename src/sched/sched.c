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

#include <kernel/sched.h>
#include <kernel/mm.h>

#define MAX_THREADS 64

static struct thread threads[MAX_THREADS];
static uint32_t current_thread = 0;
static uint32_t next_thread_id = 1;

int sched_create_thread(void (*entry_point)(void)) {
    for (int i = 0; i < MAX_THREADS; i++) {
        if (threads[i].state == THREAD_TERMINATED || threads[i].id == 0) {
            threads[i].id = next_thread_id++;
            threads[i].state = THREAD_READY;
            threads[i].priority = 1;
            
            // Allocate stack
            void* stack = mm_alloc_page();
            if (!stack) return -1;
            
            threads[i].stack_ptr = (uint32_t)stack + PAGE_SIZE - 4;
            *(uint32_t*)threads[i].stack_ptr = (uint32_t)entry_point;
            
            return threads[i].id;
        }
    }
    return -1;
}

void sched_yield(void) {
    sched_schedule();
}

void sched_schedule(void) {
    static uint32_t last_scheduled = 0;
    uint32_t next = (last_scheduled + 1) % MAX_THREADS;
    
    // Find next ready thread
    for (int i = 0; i < MAX_THREADS; i++) {
        if (threads[next].state == THREAD_READY && threads[next].id != 0) {
            // Switch context
            if (current_thread < MAX_THREADS && threads[current_thread].id != 0) {
                threads[current_thread].state = THREAD_READY;
            }
            
            threads[next].state = THREAD_RUNNING;
            last_scheduled = current_thread = next;
            
            // Context switch would happen here in real implementation
            return;
        }
        next = (next + 1) % MAX_THREADS;
    }
}

// Missing functions needed by syscall.c
struct thread* sched_get_process_by_pid(uint32_t pid) {
    for (int i = 0; i < MAX_THREADS; i++) {
        if (threads[i].id == pid) {
            return &threads[i];
        }
    }
    return 0;
}

uint32_t sched_get_current_pid(void) {
    return threads[current_thread].id;
}

struct thread* sched_get_current_process(void) {
    return &threads[current_thread];
}