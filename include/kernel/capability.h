/*
 * E-comOS Microkernel - Capability system
 */

#ifndef KERNEL_CAPABILITY_H
#define KERNEL_CAPABILITY_H

#include <stdint.h>

typedef uint32_t capability_t;

// Capability types
#define CAP_TYPE_MEMORY     1
#define CAP_TYPE_IRQ        2
#define CAP_TYPE_IPC        3
#define CAP_TYPE_IO_PORT    4

// Capability rights
#define CAP_RIGHT_READ      (1 << 0)
#define CAP_RIGHT_WRITE     (1 << 1)
#define CAP_RIGHT_EXECUTE   (1 << 2)
#define CAP_RIGHT_GRANT     (1 << 3)

struct capability {
    uint32_t type;
    uint32_t rights;
    uint32_t object_id;
    uint32_t base_addr;
    uint32_t size;
};

// Capability operations
int cap_grant(thread_id_t target, capability_t cap);
int cap_revoke(capability_t cap);
int cap_check(capability_t cap, uint32_t required_rights);

#endif