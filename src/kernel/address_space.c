/*
    E-comOS Kernel - Address Space Management
    Copyright (C) 2025,2026  Saladin5101
*/

#include <kernel/address_space.h>
#include <kernel/mm.h>

#define MAX_ADDRESS_SPACES 16

static uint8_t asUsed[MAX_ADDRESS_SPACES] = {0};

AddressSpace asCreate(void) {
    for (int i = 1; i < MAX_ADDRESS_SPACES; i++) {
        if (!asUsed[i]) {
            asUsed[i] = 1;
            return (AddressSpace)i;
        }
    }
    return 0;
}

int asDestroy(AddressSpace as) {
    if (as == 0 || as >= MAX_ADDRESS_SPACES)
        return -1;
    asUsed[as] = 0;
    return 0;
}

int asMap(AddressSpace as, uint32_t vaddr, uint32_t paddr,
          uint32_t size, uint32_t flags) {
    if (as == 0 || as >= MAX_ADDRESS_SPACES || !asUsed[as])
        return -1;
    uint32_t pages = (size + PAGE_SIZE - 1) / PAGE_SIZE;
    for (uint32_t i = 0; i < pages; i++) {
        int rc = mmMapPage(vaddr + i * PAGE_SIZE,
                           paddr + i * PAGE_SIZE, flags);
        if (rc != 0)
            return rc;
    }
    return 0;
}

int asUnmap(AddressSpace as, uint32_t vaddr, uint32_t size) {
    (void)as;
    (void)vaddr;
    (void)size;
    /* mmUnmapPage not yet wired to page tables — placeholder */
    return 0;
}
