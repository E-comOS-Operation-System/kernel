/*
    E-comOS Kernel - Scheduler
    Copyright (C) 2025,2026  Saladin5101

    Invariant: threads[current_thread].state == THREAD_RUNNING
               at all times after the first sched_schedule() call.
*/

#include <kernel/sched.h>
#include <kernel/mm.h>
#include <kernel/internal/types.h>

static Thread   threads[MAX_THREADS];
static uint32_t current_thread = 0;
static uint32_t next_thread_id  = 1;

/*
 * sched_create_thread
 *
 * Precondition:  entry_point != NULL.
 * Postcondition: new thread is in THREAD_READY state with a valid stack.
 * Returns thread ID (> 0) on success, -1 on failure.
 */
int sched_create_thread(void (*entry_point)(void)) {
    if (!entry_point)
        return -1;

    for (int i = 0; i < MAX_THREADS; i++) {
        if (threads[i].state == THREAD_TERMINATED || threads[i].id == 0) {
            /* Zero all fields first to avoid uninitialised reads (F-13) */
            threads[i].id          = 0;
            threads[i].state       = THREAD_TERMINATED;
            threads[i].stack_ptr    = 0;
            threads[i].priority    = 0;
            threads[i].block_reason = 0;
            threads[i].last_error   = 0;
            threads[i].block_data.irq_num = 0;

            void *stack = mm_alloc_page();
            if (!stack)
                return -1;

            /* Stack grows downward; place return address at top - 8.
             * Use uintptr_t throughout to avoid 32-bit truncation (F-06). */
            uintptr_t stack_top = (uintptr_t)stack + PAGE_SIZE - 8u;
            *(uint64_t *)stack_top = (uint64_t)(uintptr_t)entry_point;

            threads[i].stack_ptr = (uint32_t)stack_top; /* stored as hint only */
            threads[i].id       = next_thread_id++;
            threads[i].state    = THREAD_READY;
            threads[i].priority = 1;

            return (int)threads[i].id;
        }
    }
    return -1;
}

void sched_yield(void) {
    sched_schedule();
}

void sched_schedule(void) {
    static uint32_t last_scheduled = 0;
    uint32_t next = (last_scheduled + 1u) % MAX_THREADS;
    for (int i = 0; i < MAX_THREADS; i++) {
        if (threads[next].state == THREAD_READY && threads[next].id != 0) {
            if (current_thread < MAX_THREADS && threads[current_thread].id != 0)
                threads[current_thread].state = THREAD_READY;
            threads[next].state = THREAD_RUNNING;
            last_scheduled = current_thread = next;
            return;
        }
        next = (next + 1u) % MAX_THREADS;
    }
}

Thread *sched_get_thread_by_pid(uint32_t pid) {
    for (int i = 0; i < MAX_THREADS; i++)
        if (threads[i].id == pid)
            return &threads[i];
    return 0;
}

uint32_t sched_get_current_pid(void) {
    return threads[current_thread].id;
}

Thread *sched_get_current_thread(void) {
    return &threads[current_thread];
}
