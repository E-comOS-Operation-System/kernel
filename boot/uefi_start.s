/*
    UEFI Bootloader Entry Point
    This file defines the `_start` function, which is the entry point for the kernel.
*/

.section .text
.global _start

_start:
    /* Set up the stack pointer */
    mov $0x70000, %rsp

    /* Call the kernel main function */
    call kernel_main

    /* Halt the CPU if kernel_main returns */
.hang:
    hlt
    jmp .hang