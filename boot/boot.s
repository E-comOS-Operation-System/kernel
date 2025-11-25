# E-comOS Microkernel Boot Loader
# Boot-to-kernel transition code

.section .multiboot
    .align 4
    .long 0x1BADB002          # Multiboot magic number
    .long 0x00000003          # Multiboot flags (memory info + modules)
    .long -(0x1BADB002 + 0x00000003)  # Checksum

.section .bss
    .align 16
stack_bottom:
    .skip 16384               # 16KB kernel stack
stack_top:

.section .text
.global _start
.type _start, @function

_start:
    # Set up kernel stack pointer
    mov $stack_top, %esp
    
    # Clear direction flag for string operations
    cld
    
    # Preserve multiboot information for kernel
    push %ebx                 # Multiboot info structure pointer
    push %eax                 # Multiboot magic number
    
    # Transfer control to kernel main function
    call kernel_main
    
    # If kernel returns, halt system
halt_loop:
    cli                       # Disable interrupts
    hlt                       # Halt processor
    jmp halt_loop             # Infinite loop

.size _start, . - _start