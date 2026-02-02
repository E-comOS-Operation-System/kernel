/*
 * UEFI Bootloader Entry Point - 64-bit version
 * This file defines the `_start` function, which is the entry point for the kernel.
 */

.text
.global _start
.extern kernel_main

_start:
    /* Set up the stack pointer */
    mov $0x70000, %rsp

    /* Clear direction flag */
    cld
    
    /* Clear registers */
    xor %rax, %rax
    xor %rbx, %rbx
    xor %rcx, %rcx
    xor %rdx, %rdx
    xor %rsi, %rsi
    xor %rdi, %rdi
    xor %rbp, %rbp
    xor %r8, %r8
    xor %r9, %r9
    xor %r10, %r10
    xor %r11, %r11
    xor %r12, %r12
    xor %r13, %r13
    xor %r14, %r14
    xor %r15, %r15

    /* Call the kernel main function */
    call kernel_main

    /* Halt the CPU if kernel_main returns */
.hang:
    hlt
    jmp .hang