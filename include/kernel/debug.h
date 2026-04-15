/*
    E-comOS Kernel - Debug Utilities
    Copyright (C) 2025,2026  Saladin5101
*/

#ifndef KERNEL_DEBUG_H
#define KERNEL_DEBUG_H

void earlyDebugInit(void);
void earlyDebugPuts(const char *str);
void kernelPanic(const char *msg);
void kernelLog(const char *msg);

#endif
