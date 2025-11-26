/*
 * E-comOS Microkernel - Service system (万物皆服务)
 */

#ifndef KERNEL_SERVICE_H
#define KERNEL_SERVICE_H

#include <stdint.h>
#include <kernel/process.h>

typedef uint32_t service_id_t;

// Service registry (everything is a service)
#define SERVICE_VGA_DISPLAY     1
#define SERVICE_KEYBOARD_INPUT  2
#define SERVICE_FILE_SYSTEM     3
#define SERVICE_NETWORK         4
#define SERVICE_MEMORY_MANAGER  5

struct service {
    service_id_t id;
    process_id_t provider_process;
    char name[32];
    uint32_t capabilities;
};

// Service operations
int service_register(service_id_t id, process_id_t provider, const char *name);
int service_unregister(service_id_t id);
process_id_t service_lookup(service_id_t id);
int service_call(service_id_t id, void *request, void *response);

#endif