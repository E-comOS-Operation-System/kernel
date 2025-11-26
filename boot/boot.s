; E-comOS Microkernel Boot Loader
; Boot-to-kernel transition code

section .multiboot
    align 4
    dd 0x1BADB002          ; Multiboot magic number
    dd 0x00000003          ; Multiboot flags (memory info + modules)
    dd -(0x1BADB002 + 0x00000003)  ; Checksum

section .bss
    align 16
stack_bottom:
    resb 16384             ; 16KB kernel stack
stack_top:

section .text
global _start

_start:
    ; Set up kernel stack pointer
    mov esp, stack_top
    
    ; Clear direction flag for string operations
    cld
    
    ; Preserve multiboot information for kernel
    push ebx               ; Multiboot info structure pointer
    push eax               ; Multiboot magic number
    
    ; Transfer control to kernel main function
    extern kernel_main
    call kernel_main
    
    ; If kernel returns, halt system
halt_loop:
    cli                    ; Disable interrupts
    hlt                    ; Halt processor
    jmp halt_loop          ; Infinite loop