/*
    E-comOS Kernel - Process System
    Copyright (C) 2025,2026  Saladin5101
*/

#ifndef KERNEL_PROCESS_H
#define KERNEL_PROCESS_H

#include <stdint.h>

typedef uint32_t ProcessId;

#define PROCESS_TYPE_KERNEL  0
#define PROCESS_TYPE_SERVICE 1
#define PROCESS_TYPE_DRIVER  2
#define PROCESS_TYPE_USER    3

typedef struct {
    ProcessId id;
    uint32_t  type;
    uint32_t  addressSpace;
    uint32_t  state;
    uint8_t   blockReason;
    int32_t   lastError;
    union {
        uint8_t irqNum;
    } blockData;
} Process;

ProcessId processCreate(uint32_t type, void *entryPoint);
int       processDestroy(ProcessId pid);

#endif
