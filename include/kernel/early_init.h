/*
    E-comOS Kernel - Early Initialization
    Copyright (C) 2025,2026  Saladin5101
*/

#ifndef KERNEL_EARLY_INIT_H
#define KERNEL_EARLY_INIT_H

#include <stdint.h>

int earlyKernelInit(uint32_t multibootMagic, uint32_t multibootInfo);

#endif
