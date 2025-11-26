/*
 * E-comOS Microkernel - Address space management
 */

#ifndef KERNEL_ADDRESS_SPACE_H
#define KERNEL_ADDRESS_SPACE_H

#include <stdint.h>

typedef uint32_t address_space_t;

// Address space operations (microkernel only)
address_space_t as_create(void);
int as_destroy(address_space_t as);
int as_map(address_space_t as, uint32_t vaddr, uint32_t paddr, uint32_t size, uint32_t flags);
int as_unmap(address_space_t as, uint32_t vaddr, uint32_t size);

#endif