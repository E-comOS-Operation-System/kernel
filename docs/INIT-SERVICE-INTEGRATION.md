# Init Service Integration Guide

## Overview

`init_service_entry` is the first process created by the kernel (TID 1).
It runs briefly in ring 0 to set up its service registry, then drops to
ring 3 via `iret` and enters its IPC loop.

Every external component (EBTS, display service, FS service, …) lives in
its own repository.  The kernel knows nothing about them.  They register
themselves with init over IPC, and clients discover them the same way.

---

## Kernel side (this repository)

The kernel only needs to:

1. Set up GDT with ring-3 segments + TSS (`gdt_init`)
2. Set up IDT with all ISR/IRQ gates + `int 0x80` syscall gate (`idt_init`)
3. Wire timer IRQ → `time_tick()` (`irq_init_timer`)
4. Create init thread: `sched_create_thread(init_service_entry)`
5. Enable interrupts and enter the scheduler loop

All of this is already done in `src/kernel/main.c`.

---

## Answers to the four boot questions

| Question | Answer |
|---|---|
| Kernel format | Flat binary (`kernel.bin`). No ELF parsing needed. |
| Entry convention | No parameters. Bootloader jumps directly to `_start` in `boot/uefi_start.s`, which calls `kernel_main()` with no arguments. |
| Load address | `0x100000` (1 MB). Set in `uefi.ld`: `. = 0x100000`. |
| ExitBootServices | Not required. The kernel does not use UEFI runtime services. The bootloader sets up the stack at `0x70000` and jumps straight to `_start`. |

---

## Userspace side (external repositories)

### System call ABI

```c
// int 0x80 calling convention
// eax = syscall number
// ebx = arg1, ecx = arg2, edx = arg3
// return value in eax

static inline long syscall(uint32_t num,
                            uint32_t a1, uint32_t a2, uint32_t a3) {
    long ret;
    asm volatile("int $0x80"
                 : "=a"(ret)
                 : "a"(num), "b"(a1), "c"(a2), "d"(a3)
                 : "memory");
    return ret;
}

#define SYS_IPC_SEND     1
#define SYS_IPC_RECEIVE  2
#define SYS_THREAD_YIELD 3
#define SYS_ADDRESS_MAP  4
#define SYS_IRQ_WAIT     5
```

### Registering a service with init

Init's PID is always **1**.

```c
#define MSG_SERVICE_REGISTER 0x01

void register_with_init(uint32_t my_service_id) {
    ipc_message_t msg = {0};
    msg.type   = MSG_SERVICE_REGISTER;
    msg.target = 1; /* init PID */
    /* encode service_id little-endian into data[0..3] */
    msg.data[0] = my_service_id & 0xFF;
    msg.data[1] = (my_service_id >> 8)  & 0xFF;
    msg.data[2] = (my_service_id >> 16) & 0xFF;
    msg.data[3] = (my_service_id >> 24) & 0xFF;
    msg.size = 4;
    syscall(SYS_IPC_SEND, 1, (uint32_t)&msg, 0);
}
```

### Looking up a service

```c
#define MSG_SERVICE_LOOKUP 0x02
#define MSG_SERVICE_REPLY  0x03

uint32_t lookup_service(uint32_t service_id) {
    ipc_message_t msg = {0};
    msg.type   = MSG_SERVICE_LOOKUP;
    msg.target = 1;
    msg.data[0] = service_id & 0xFF;
    msg.data[1] = (service_id >> 8)  & 0xFF;
    msg.data[2] = (service_id >> 16) & 0xFF;
    msg.data[3] = (service_id >> 24) & 0xFF;
    msg.size = 4;
    syscall(SYS_IPC_SEND, 1, (uint32_t)&msg, 0);

    ipc_message_t reply = {0};
    syscall(SYS_IPC_RECEIVE, (uint32_t)&reply, 0, 0);

    return reply.data[0]
         | ((uint32_t)reply.data[1] << 8)
         | ((uint32_t)reply.data[2] << 16)
         | ((uint32_t)reply.data[3] << 24);
}
```

### Well-known service IDs

Defined in `include/kernel/service.h`:

| ID | Service |
|---|---|
| 1 | VGA display |
| 2 | Keyboard input |
| 3 | File system |
| 4 | Network |
| 5 | Memory manager |

### EBTS example startup sequence

```
1. Bootloader loads kernel.bin to 0x100000, jumps to _start
2. kernel_main initialises GDT/TSS, IDT, MM, sched, IPC
3. sched_create_thread(init_service_entry) -> TID 1
4. Scheduler runs init; init drops to ring 3
5. Bootloader (or a loader service) maps EBTS binary to e.g. 0x400000
6. EBTS _start runs in ring 3
7. EBTS calls register_with_init(SERVICE_KEYBOARD_INPUT)
8. EBTS calls lookup_service(SERVICE_VGA_DISPLAY) to find display server
9. EBTS sends IPC to display server to write text
10. EBTS calls SYS_IRQ_WAIT for keyboard IRQ to read input
```

---

## File map

| File | Role |
|---|---|
| `arch/x86_64/cpu/gdt.c` | GDT (5 segments) + TSS |
| `arch/x86_64/interrupts/idt.c` | IDT: all 32 exceptions + 16 IRQs + int 0x80 |
| `arch/x86_64/interrupts/isr.s` | Exception stubs + int 0x80 syscall stub |
| `arch/x86_64/interrupts/irq.s` | Hardware IRQ stubs |
| `arch/x86_64/interrupts/irq.c` | IRQ dispatch + timer → time_tick() |
| `src/kernel/main.c` | Boot sequence, creates init thread |
| `src/kernel/init.c` | Init service: registry + ring-3 drop |
| `src/kernel/syscall.c` | int 0x80 handler dispatch |
| `src/ipc/ipc.c` | Kernel IPC queue |
| `src/time/time.c` | system_ticks counter |
