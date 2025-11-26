/*
 * E-comOS Microkernel - Process system (万物皆进程)
 */

#ifndef KERNEL_PROCESS_H
#define KERNEL_PROCESS_H

#include <stdint.h>
#include <kernel/object.h>

typedef uint32_t process_id_t;

// Process types (everything runs as a process)
#define PROCESS_TYPE_KERNEL     0  // Minimal kernel threads
#define PROCESS_TYPE_SERVICE    1  // System services (VGA, FS, etc.)
#define PROCESS_TYPE_DRIVER     2  // Device drivers
#define PROCESS_TYPE_USER       3  // User applications

struct process {
    process_id_t id;
    uint32_t type;
    uint32_t address_space;
    object_id_t *objects;      // Objects owned by this process
    uint32_t object_count;
    uint32_t state;
};

// Process operations
process_id_t process_create(uint32_t type, void *entry_point);
int process_destroy(process_id_t pid);
int process_grant_object(process_id_t pid, object_id_t obj);

#endif