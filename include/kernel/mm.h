/*
    E-comOS Kernel - A Microkernel for E-comOS
    Copyright (C) 2025,2026  Saladin5101

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as published
    by the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef KERNEL_MM_H
#define KERNEL_MM_H

#include <stdint.h>

#define PAGE_SIZE 4096

// Memory mapping flags
#define MM_FLAG_READ    (1 << 0)
#define MM_FLAG_WRITE   (1 << 1)
#define MM_FLAG_EXEC    (1 << 2)
#define MM_FLAG_USER    (1 << 3)

// Memory management functions
void* mm_alloc_page(void);
void mm_free_page(void* page);
int mm_map_page(uint32_t virtual_addr, uint32_t physical_addr, uint32_t flags);

#endif