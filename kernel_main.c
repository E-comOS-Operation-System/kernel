// E-comOS Kernel Main Entry Point
// Copyright (C) 2025 Saladin5101

#include <stdint.h>

// Forward declaration from main kernel
extern void kernel_main(void);

// Simple wrapper to call the main kernel function
void _start(void) {
    kernel_main();
}

// Include the main kernel implementation
#include "src/kernel/main.c"