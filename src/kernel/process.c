/*
    E-comOS Kernel - Process Management
    Copyright (C) 2025,2026  Saladin5101
*/

#include <kernel/process.h>
#include <kernel/sched.h>
#include <kernel/mm.h>

process_id process_create(uint32_t type, void *entry_point) {
    (void)type;
    int tid = sched_create_thread((void (*)(void))entry_point);
    if (tid < 0)
        return 0;
    return (process_id)tid;
}

int process_destroy(process_id pid) {
    Thread *t = sched_get_thread_by_pid(pid);
    if (!t)
        return -1;
    t->state = THREAD_TERMINATED;
    return 0;
}
