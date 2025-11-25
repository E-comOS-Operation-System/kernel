/*
 * E-comOS Microkernel - Early debug output
 * Minimal output for kernel debugging before userspace services
 */

#ifndef KERNEL_DEBUG_H
#define KERNEL_DEBUG_H

// Early debug output (serial or minimal VGA)
void early_debug_init(void);
void early_debug_puts(const char* str);

#endif