/*
    E-comOS Kernel - Capability System
    Copyright (C) 2025,2026  Saladin5101
*/

#include <kernel/capability.h>

int capGrant(uint32_t targetPid, Capability cap) {
    (void)targetPid;
    (void)cap;
    /* Capability transfer between processes — requires IPC integration.
       Placeholder until object manager service is implemented. */
    return 0;
}

int capRevoke(Capability cap) {
    (void)cap;
    return 0;
}

int capCheck(Capability cap, uint32_t requiredRights) {
    return (cap.rights & requiredRights) == requiredRights ? 0 : -1;
}
