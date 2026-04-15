/*
    E-comOS Kernel - Process Management
    Copyright (C) 2025,2026  Saladin5101
*/

#include <kernel/process.h>
#include <kernel/sched.h>
#include <kernel/mm.h>

ProcessId processCreate(uint32_t type, void *entryPoint) {
    (void)type;
    int tid = schedCreateThread((void (*)(void))entryPoint);
    if (tid < 0)
        return 0;
    return (ProcessId)tid;
}

int processDestroy(ProcessId pid) {
    Thread *t = schedGetThreadByPid(pid);
    if (!t)
        return -1;
    t->state = THREAD_TERMINATED;
    return 0;
}
