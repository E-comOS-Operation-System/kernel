/*
 * E-comOS Microkernel - Internal type definitions
 */

#ifndef KERNEL_INTERNAL_TYPES_H
#define KERNEL_INTERNAL_TYPES_H

#include <stdint.h>
#include <stddef.h>

// Kernel object IDs
typedef uint32_t thread_id_t;
typedef uint32_t process_id_t;
typedef uint32_t capability_id_t;

// Error codes
#define KERNEL_OK           0
#define KERNEL_ERROR       -1
#define KERNEL_INVALID_ARG -2
#define KERNEL_NO_MEMORY   -3
#define KERNEL_NO_PERM     -4

// Kernel limits
#define MAX_THREADS        256
#define MAX_PROCESSES      64
#define MAX_CAPABILITIES   1024

#endif