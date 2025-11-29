# E-comOS 64-bit Kernel Entry
# Copyright (C) 2025 Saladin5101

.section .text
.global _start
.extern kernel_main

_start:
    # Kernel entry point
    movq $0x200000, %rsp
    cld
    call kernel_main
    xorq %rax, %rax
    hlt