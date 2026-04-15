/*
    E-comOS Kernel - Scheduler
    Copyright (C) 2025,2026  Saladin5101

    Invariant: threads[currentThread].state == THREAD_RUNNING
               at all times after the first schedSchedule() call.
*/

#include <kernel/sched.h>
#include <kernel/mm.h>
#include <kernel/internal/types.h>

static Thread   threads[MAX_THREADS];
static uint32_t currentThread = 0;
static uint32_t nextThreadId  = 1;

/*
 * schedCreateThread
 *
 * Precondition:  entryPoint != NULL.
 * Postcondition: new thread is in THREAD_READY state with a valid stack.
 * Returns thread ID (> 0) on success, -1 on failure.
 */
int schedCreateThread(void (*entryPoint)(void)) {
    if (!entryPoint)
        return -1;

    for (int i = 0; i < MAX_THREADS; i++) {
        if (threads[i].state == THREAD_TERMINATED || threads[i].id == 0) {
            /* Zero all fields first to avoid uninitialised reads (F-13) */
            threads[i].id          = 0;
            threads[i].state       = THREAD_TERMINATED;
            threads[i].stackPtr    = 0;
            threads[i].priority    = 0;
            threads[i].blockReason = 0;
            threads[i].lastError   = 0;
            threads[i].blockData.irqNum = 0;

            void *stack = mmAllocPage();
            if (!stack)
                return -1;

            /* Stack grows downward; place return address at top - 8.
             * Use uintptr_t throughout to avoid 32-bit truncation (F-06). */
            uintptr_t stackTop = (uintptr_t)stack + PAGE_SIZE - 8u;
            *(uint64_t *)stackTop = (uint64_t)(uintptr_t)entryPoint;

            threads[i].stackPtr = (uint32_t)stackTop; /* stored as hint only */
            threads[i].id       = nextThreadId++;
            threads[i].state    = THREAD_READY;
            threads[i].priority = 1;

            return (int)threads[i].id;
        }
    }
    return -1;
}

void schedYield(void) {
    schedSchedule();
}

void schedSchedule(void) {
    static uint32_t lastScheduled = 0;
    uint32_t next = (lastScheduled + 1u) % MAX_THREADS;
    for (int i = 0; i < MAX_THREADS; i++) {
        if (threads[next].state == THREAD_READY && threads[next].id != 0) {
            if (currentThread < MAX_THREADS && threads[currentThread].id != 0)
                threads[currentThread].state = THREAD_READY;
            threads[next].state = THREAD_RUNNING;
            lastScheduled = currentThread = next;
            return;
        }
        next = (next + 1u) % MAX_THREADS;
    }
}

Thread *schedGetThreadByPid(uint32_t pid) {
    for (int i = 0; i < MAX_THREADS; i++)
        if (threads[i].id == pid)
            return &threads[i];
    return 0;
}

uint32_t schedGetCurrentPid(void) {
    return threads[currentThread].id;
}

Thread *schedGetCurrentThread(void) {
    return &threads[currentThread];
}
