# E-comOS x86_64 - Context switching assembly

.section .text
.global context_switch
.type context_switch, @function

# void context_switch(struct cpu_context *old_ctx, struct cpu_context *new_ctx)
context_switch:
    # Save old context
    mov 4(%esp), %eax    # old_ctx pointer
    
    # Save registers to old context
    mov %eax, 0(%eax)    # eax
    mov %ebx, 4(%eax)    # ebx
    mov %ecx, 8(%eax)    # ecx
    mov %edx, 12(%eax)   # edx
    mov %esi, 16(%eax)   # esi
    mov %edi, 20(%eax)   # edi
    mov %esp, 24(%eax)   # esp
    mov %ebp, 28(%eax)   # ebp
    
    # Save return address as eip
    mov (%esp), %ebx
    mov %ebx, 32(%eax)   # eip
    
    # Save flags
    pushf
    pop %ebx
    mov %ebx, 36(%eax)   # eflags
    
    # Load new context
    mov 8(%esp), %eax    # new_ctx pointer
    
    # Restore registers from new context
    mov 4(%eax), %ebx    # ebx
    mov 8(%eax), %ecx    # ecx
    mov 12(%eax), %edx   # edx
    mov 16(%eax), %esi   # esi
    mov 20(%eax), %edi   # edi
    mov 28(%eax), %ebp   # ebp
    
    # Restore flags
    mov 36(%eax), %ebx   # eflags
    push %ebx
    popf
    
    # Restore stack pointer and jump to new eip
    mov 24(%eax), %esp   # esp
    mov 32(%eax), %ebx   # eip
    mov 0(%eax), %eax    # eax (restore last)
    
    jmp *%ebx            # Jump to new eip

.size context_switch, . - context_switch