/*
    E-comOS Kernel - Address Space Management
    Copyright (C) 2025,2026  Saladin5101
*/

#ifndef KERNEL_ADDRESS_SPACE_H
#define KERNEL_ADDRESS_SPACE_H

#include <stdint.h>

typedef uint32_t AddressSpace;

AddressSpace asCreate(void);
int          asDestroy(AddressSpace as);
int          asMap(AddressSpace as, uint32_t vaddr, uint32_t paddr,
                   uint32_t size, uint32_t flags);
int          asUnmap(AddressSpace as, uint32_t vaddr, uint32_t size);

#endif
