# E-comOS - ISR stubs (64-bit)

.section .text

# Save all GP registers (no pusha in 64-bit mode)
.macro SAVE_REGS
    pushq %rax
    pushq %rbx
    pushq %rcx
    pushq %rdx
    pushq %rsi
    pushq %rdi
    pushq %rbp
    pushq %r8
    pushq %r9
    pushq %r10
    pushq %r11
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15
.endm

.macro RESTORE_REGS
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %r11
    popq %r10
    popq %r9
    popq %r8
    popq %rbp
    popq %rdi
    popq %rsi
    popq %rdx
    popq %rcx
    popq %rbx
    popq %rax
.endm

.macro ISR_NOERRCODE num
.global isr\num
isr\num:
    cli
    pushq $0
    pushq $\num
    jmp isr_common_stub
.endm

.macro ISR_ERRCODE num
.global isr\num
isr\num:
    cli
    pushq $\num
    jmp isr_common_stub
.endm

ISR_NOERRCODE 0
ISR_NOERRCODE 1
ISR_NOERRCODE 2
ISR_NOERRCODE 3
ISR_NOERRCODE 4
ISR_NOERRCODE 5
ISR_NOERRCODE 6
ISR_NOERRCODE 7
ISR_ERRCODE   8
ISR_NOERRCODE 9
ISR_ERRCODE   10
ISR_ERRCODE   11
ISR_ERRCODE   12
ISR_ERRCODE   13
ISR_ERRCODE   14
ISR_NOERRCODE 15
ISR_NOERRCODE 16
ISR_ERRCODE   17
ISR_NOERRCODE 18
ISR_NOERRCODE 19
ISR_NOERRCODE 20
ISR_NOERRCODE 21
ISR_NOERRCODE 22
ISR_NOERRCODE 23
ISR_NOERRCODE 24
ISR_NOERRCODE 25
ISR_NOERRCODE 26
ISR_NOERRCODE 27
ISR_NOERRCODE 28
ISR_NOERRCODE 29
ISR_NOERRCODE 30
ISR_NOERRCODE 31

# int 0x80 syscall entry
.global isr128
isr128:
    cli
    pushq $0
    pushq $128
    jmp syscall_stub

syscall_stub:
    SAVE_REGS

    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs

    # isr frame on stack (from bottom): r15..rax, int_no, err_code, rip, cs, rflags, rsp, ss
    # int_no is at rsp+120 (15 regs * 8), err_code at rsp+128 — but we need
    # the original rax/rbx/rcx/rdx which are at known offsets in SAVE_REGS frame.
    # syscall ABI: rax=num, rbx=arg1, rcx=arg2, rdx=arg3
    movq 112(%rsp), %rdi   # saved rax -> arg0 (syscall number)
    movq 104(%rsp), %rsi   # saved rbx -> arg1
    movq  96(%rsp), %rdx   # saved rcx -> arg2
    movq  88(%rsp), %rcx   # saved rdx -> arg3

    call syscall_handler

    # store return value back into saved rax slot
    movq %rax, 112(%rsp)

    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs

    RESTORE_REGS
    addq $16, %rsp          # pop err_code + int_no
    sti
    iretq

isr_common_stub:
    SAVE_REGS

    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs

    # isr_handler(int_no, err_code) — pass as two args
    movq 120(%rsp), %rdi    # int_no  (after 15 pushq = 120 bytes)
    movq 128(%rsp), %rsi    # err_code

    call isr_handler

    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs

    RESTORE_REGS
    addq $16, %rsp
    sti
    iretq
