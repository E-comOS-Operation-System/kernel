/*
    E-comOS Kernel - Service Registry
    Copyright (C) 2025,2026  Saladin5101
*/

#include <kernel/service.h>

#define MAX_SERVICES 32

static Service serviceTable[MAX_SERVICES];
static int     serviceCount = 0;

int serviceRegister(ServiceId id, uint32_t providerPid, const char *name) {
    if (serviceCount >= MAX_SERVICES)
        return -1;
    for (int i = 0; i < serviceCount; i++) {
        if (serviceTable[i].id == id)
            return -2; /* already registered */
    }
    serviceTable[serviceCount].id          = id;
    serviceTable[serviceCount].providerPid = providerPid;
    serviceTable[serviceCount].capabilities = 0;
    /* copy name */
    int j = 0;
    while (name[j] && j < 31) {
        serviceTable[serviceCount].name[j] = name[j];
        j++;
    }
    serviceTable[serviceCount].name[j] = '\0';
    serviceCount++;
    return 0;
}

int serviceUnregister(ServiceId id) {
    for (int i = 0; i < serviceCount; i++) {
        if (serviceTable[i].id == id) {
            serviceTable[i] = serviceTable[--serviceCount];
            return 0;
        }
    }
    return -1;
}

uint32_t serviceLookup(ServiceId id) {
    for (int i = 0; i < serviceCount; i++) {
        if (serviceTable[i].id == id)
            return serviceTable[i].providerPid;
    }
    return 0;
}

int serviceCall(ServiceId id, void *request, void *response) {
    (void)id;
    (void)request;
    (void)response;
    /* Full implementation requires IPC to the provider process.
       Userspace services use SYS_IPC_SEND/RECEIVE directly. */
    return -1;
}
