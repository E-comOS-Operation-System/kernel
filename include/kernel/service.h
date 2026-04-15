/*
    E-comOS Kernel - Service Registry
    Copyright (C) 2025,2026  Saladin5101
*/

#ifndef KERNEL_SERVICE_H
#define KERNEL_SERVICE_H

#include <stdint.h>

typedef uint32_t ServiceId;

#define SERVICE_VGA_DISPLAY    1
#define SERVICE_KEYBOARD_INPUT 2
#define SERVICE_FILE_SYSTEM    3
#define SERVICE_NETWORK        4
#define SERVICE_MEMORY_MANAGER 5

typedef struct {
    ServiceId id;
    uint32_t  providerPid;
    char      name[32];
    uint32_t  capabilities;
} Service;

int       serviceRegister(ServiceId id, uint32_t providerPid, const char *name);
int       serviceUnregister(ServiceId id);
uint32_t  serviceLookup(ServiceId id);
int       serviceCall(ServiceId id, void *request, void *response);

#endif
