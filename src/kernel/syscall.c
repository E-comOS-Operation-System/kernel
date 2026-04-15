/*
    E-comOS Kernel - Syscall handler
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

#include <kernel/syscall.h>
#include <kernel/ipc.h>
#include <kernel/sched.h>
#include <kernel/mm.h>
#include <kernel/time.h>
#include <stdint.h>

#define MAX_IRQ_WAITERS 16
#define MAX_IRQS        16

typedef struct {
    uint32_t pid;
    uint8_t  irqNumber;
    uint8_t  flags;
    uint32_t timeoutMs;
    uint64_t startTime;
    uint8_t  isActive;
} IrqWaiter;

static IrqWaiter     irqWaiters[MAX_IRQ_WAITERS];
static uint32_t      numWaiters = 0;
static volatile uint32_t irqOccurred[MAX_IRQS];
static volatile uint32_t irqOccurrenceCount[MAX_IRQS];

void syscallIrqInit(void) {
    for (int i = 0; i < MAX_IRQ_WAITERS; i++) {
        irqWaiters[i].pid      = 0;
        irqWaiters[i].isActive = 0;
    }
    for (int i = 0; i < MAX_IRQS; i++) {
        irqOccurred[i]         = 0;
        irqOccurrenceCount[i]  = 0;
    }
    numWaiters = 0;
}

static int findIrqWaiter(uint32_t pid, uint8_t irqNum) {
    for (uint32_t i = 0; i < numWaiters; i++) {
        if (irqWaiters[i].isActive &&
            irqWaiters[i].pid == pid &&
            irqWaiters[i].irqNumber == irqNum)
            return (int)i;
    }
    return -1;
}

static int addIrqWaiter(uint32_t pid, uint8_t irqNum,
                        uint8_t flags, uint32_t timeoutMs) {
    if (findIrqWaiter(pid, irqNum) >= 0)
        return -2;
    for (uint32_t i = 0; i < MAX_IRQ_WAITERS; i++) {
        if (!irqWaiters[i].isActive) {
            irqWaiters[i].pid       = pid;
            irqWaiters[i].irqNumber = irqNum;
            irqWaiters[i].flags     = flags;
            irqWaiters[i].timeoutMs = timeoutMs;
            irqWaiters[i].startTime = timeGetCurrentMs();
            irqWaiters[i].isActive  = 1;
            if (i >= numWaiters)
                numWaiters = i + 1;
            return 0;
        }
    }
    return -1;
}

static void removeIrqWaiter(uint32_t pid, uint8_t irqNum) {
    int idx = findIrqWaiter(pid, irqNum);
    if (idx >= 0)
        irqWaiters[idx].isActive = 0;
}

void syscallIrqNotify(uint8_t irqNum) {
    if (irqNum >= MAX_IRQS)
        return;
    irqOccurred[irqNum] = 1;
    irqOccurrenceCount[irqNum]++;
    for (uint32_t i = 0; i < numWaiters; i++) {
        if (!irqWaiters[i].isActive) continue;
        if (irqWaiters[i].irqNumber != irqNum) continue;
        irqWaiters[i].isActive = 0;
        Thread *t = schedGetThreadByPid(irqWaiters[i].pid);
        if (t && t->state == THREAD_BLOCKED) {
            t->state       = THREAD_READY;
            t->blockReason = BLOCK_REASON_NONE;
        }
    }
}

void syscallIrqCheckTimeouts(void) {
    uint64_t now = timeGetCurrentMs();
    for (uint32_t i = 0; i < numWaiters; i++) {
        if (!irqWaiters[i].isActive) continue;
        if (irqWaiters[i].timeoutMs == 0) continue;
        uint64_t elapsed = now - irqWaiters[i].startTime;
        if (elapsed < irqWaiters[i].timeoutMs) continue;
        uint32_t pid = irqWaiters[i].pid;
        irqWaiters[i].isActive = 0;
        Thread *t = schedGetThreadByPid(pid);
        if (t && t->state == THREAD_BLOCKED) {
            t->state       = THREAD_READY;
            t->blockReason = BLOCK_REASON_NONE;
            t->lastError   = ERR_TIMEOUT;
        }
    }
}

static long irqWaitSyscall(uint8_t irqNum, uint8_t flags, uint32_t timeoutMs) {
    if (irqNum >= MAX_IRQS) return -1;
    uint32_t pid = schedGetCurrentPid();
    if (pid == 0) return -4;
    if (irqOccurred[irqNum]) {
        if (flags & IRQ_WAIT_CLEAR)
            irqOccurred[irqNum] = 0;
        return 0;
    }
    if (flags & IRQ_WAIT_NOWAIT) return -2;
    int rc = addIrqWaiter(pid, irqNum, flags, timeoutMs);
    if (rc < 0) return rc;
    Thread *t = schedGetCurrentThread();
    if (!t) {
        removeIrqWaiter(pid, irqNum);
        return -4;
    }
    t->state                = THREAD_BLOCKED;
    t->blockReason          = BLOCK_REASON_IRQ_WAIT;
    t->blockData.irqNum     = irqNum;
    while (!irqOccurred[irqNum]) {
        syscallIrqCheckTimeouts();
        if (findIrqWaiter(pid, irqNum) < 0)
            break;
        schedYield();
    }
    if (irqOccurred[irqNum] && (flags & IRQ_WAIT_CLEAR))
        irqOccurred[irqNum] = 0;
    removeIrqWaiter(pid, irqNum);
    if (t->lastError == ERR_TIMEOUT) {
        t->lastError = 0;
        return -3;
    }
    return 0;
}

long syscallHandler(uint32_t num, uint32_t arg1, uint32_t arg2, uint32_t arg3) {
    switch (num) {
    case SYS_IPC_SEND:
        return ipcSend((ThreadId)arg1, (IpcMessage *)(uintptr_t)arg2);
    case SYS_IPC_RECEIVE:
        return ipcReceive((IpcMessage *)(uintptr_t)arg1);
    case SYS_THREAD_YIELD:
        schedYield();
        return 0;
    case SYS_ADDRESS_MAP:
        return mmMapPage(arg1, arg2, arg3);
    case SYS_IRQ_WAIT:
        return irqWaitSyscall((uint8_t)arg1, (uint8_t)arg2, arg3);
    case SYS_IRQ_GET_COUNT:
        if (arg1 >= MAX_IRQS) return -1;
        return (long)irqOccurrenceCount[arg1];
    case SYS_IRQ_RESET_COUNT:
        if (arg1 >= MAX_IRQS) return -1;
        { uint32_t old = irqOccurrenceCount[arg1]; irqOccurrenceCount[arg1] = 0; return old; }
    default:
        return -1;
    }
}
