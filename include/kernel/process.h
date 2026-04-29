/*
    E-comOS Kernel - Process System
    Copyright (C) 2025,2026  Saladin5101
*/

#ifndef KERNEL_PROCESS_H
#define KERNEL_PROCESS_H

#include <stdint.h>

typedef process_id;

#define PROCESS_TYPE_KERNEL  0
#define PROCESS_TYPE_SERVICE 1
#define PROCESS_TYPE_DRIVER  2
#define PROCESS_TYPE_USER    3

typedef struct {
    process_id id;
    uint32_t  type;
    uint32_t  address_space;
    uint32_t  state;
    uint8_t   block_reason;
    int32_t   last_error;
    union {
        uint8_t irq_num;
    } block_data;
} Process;

process_id process_create(uint32_t type, void *entry_point);
int       process_destroy(process_id pid);

#endif
