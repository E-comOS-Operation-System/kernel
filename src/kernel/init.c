/*
    E-comOS Init Service - First userspace process (PID 1)
    Copyright (C) 2025,2026 Saladin5101

    Responsibilities:
      - Receive service-registration IPC messages from userspace servers
      - Maintain a simple service table (id -> source PID)
      - Forward lookup replies
      - Drop to ring 3 before entering the main loop so that all subsequent
        IPC is done through int 0x80, not direct kernel calls

    This file is compiled into the kernel image and linked at kernel load
    time.  It is the ONLY piece of "userspace" that lives inside the kernel
    binary; every other service (EBTS, display, FS …) lives in its own
    repository and is loaded by the bootloader or by init itself via
    SYS_ADDRESS_MAP + a jump.
*/

#include <stdint.h>
#include <kernel/ipc.h>
#include <kernel/syscall.h>
#include <kernel/printkit/print.h>

/* ------------------------------------------------------------------ */
/* IPC message types understood by init                                */
/* ------------------------------------------------------------------ */
#define MSG_SERVICE_REGISTER  0x01  /* server -> init: "I provide service X" */
#define MSG_SERVICE_LOOKUP    0x02  /* client -> init: "who provides service X?" */
#define MSG_SERVICE_REPLY     0x03  /* init   -> client: reply with provider PID */

/* ------------------------------------------------------------------ */
/* Simple service registry                                             */
/* ------------------------------------------------------------------ */
#define MAX_SERVICES 32

typedef struct {
    uint32_t service_id;
    uint32_t provider_pid;
    uint8_t  active;
} service_entry_t;

static service_entry_t service_table[MAX_SERVICES];

static void registry_init(void) {
    for (int i = 0; i < MAX_SERVICES; i++)
        service_table[i].active = 0;
}

static int registry_register(uint32_t service_id, uint32_t provider_pid) {
    for (int i = 0; i < MAX_SERVICES; i++) {
        if (!service_table[i].active) {
            service_table[i].service_id   = service_id;
            service_table[i].provider_pid = provider_pid;
            service_table[i].active       = 1;
            return 0;
        }
    }
    return -1; /* table full */
}

static uint32_t registry_lookup(uint32_t service_id) {
    for (int i = 0; i < MAX_SERVICES; i++) {
        if (service_table[i].active &&
            service_table[i].service_id == service_id)
            return service_table[i].provider_pid;
    }
    return 0; /* not found */
}

/* ------------------------------------------------------------------ */
/* Drop to ring 3 via iret                                             */
/*                                                                     */
/* Selectors (must match gdt.c):                                       */
/*   0x1B = user code  (index 3, RPL 3)                               */
/*   0x23 = user data  (index 4, RPL 3)                               */
/*                                                                     */
/* The stack passed in must already be mapped with MM_FLAG_USER_RW.   */
/* entry_point must be a virtual address accessible from ring 3.      */
/* ------------------------------------------------------------------ */
static void __attribute__((noreturn))
drop_to_usermode(uint64_t entry_point, uint64_t user_stack_top)
{
    /*
     * 64-bit iretq frame (pushed in reverse order):
     *   [rsp+32] ss
     *   [rsp+24] rsp (user stack)
     *   [rsp+16] rflags
     *   [rsp+ 8] cs
     *   [rsp+ 0] rip
     *
     * Selectors (must match gdt.c):
     *   0x1B = user code  (index 3, RPL 3)
     *   0x23 = user data  (index 4, RPL 3)
     */
    __asm__ volatile (
        "cli\n"
        "movw $0x23, %%ax\n"
        "movw %%ax, %%ds\n"
        "movw %%ax, %%es\n"
        "movw %%ax, %%fs\n"
        "movw %%ax, %%gs\n"
        "pushq $0x23\n"          /* ss  */
        "pushq %[sp]\n"          /* rsp */
        "pushfq\n"
        "orq  $0x200, (%%rsp)\n" /* IF=1 */
        "pushq $0x1B\n"          /* cs  */
        "pushq %[ip]\n"          /* rip */
        "iretq\n"
        :
        : [ip] "r"(entry_point), [sp] "r"(user_stack_top)
        : "rax", "memory"
    );
    __builtin_unreachable();
}

/* ------------------------------------------------------------------ */
/* Inline syscall wrappers — used after drop_to_usermode (ring 3)     */
/* ------------------------------------------------------------------ */
static inline long _sys_ipc_send(uint32_t target, ipc_message_t *msg) {
    long ret;
    __asm__ volatile ("int $0x80"
        : "=a"(ret)
        : "a"(SYS_IPC_SEND), "b"(target), "c"((uint32_t)(uintptr_t)msg), "d"(0)
        : "memory");
    return ret;
}

static inline long _sys_ipc_receive(ipc_message_t *msg) {
    long ret;
    __asm__ volatile ("int $0x80"
        : "=a"(ret)
        : "a"(SYS_IPC_RECEIVE), "b"((uint32_t)(uintptr_t)msg), "c"(0), "d"(0)
        : "memory");
    return ret;
}

static inline void _sys_yield(void) {
    __asm__ volatile ("int $0x80"
        : : "a"(SYS_THREAD_YIELD), "b"(0), "c"(0), "d"(0) : "memory");
}
/* ------------------------------------------------------------------ */
/* Main IPC dispatch loop (runs in ring 3 after drop_to_usermode)     */
/* ------------------------------------------------------------------ */
static void __attribute__((noreturn)) init_loop(void) {
    ipc_message_t msg;

    while (1) {
        long rc = _sys_ipc_receive(&msg);
        if (rc != ECLIB_OK) {
            _sys_yield();
            continue;
        }

        switch (msg.type) {
        case MSG_SERVICE_REGISTER: {
            uint32_t svc_id = msg.data[0] |
                              ((uint32_t)msg.data[1] << 8) |
                              ((uint32_t)msg.data[2] << 16) |
                              ((uint32_t)msg.data[3] << 24);
            registry_register(svc_id, msg.source);
            break;
        }
        case MSG_SERVICE_LOOKUP: {
            uint32_t svc_id = msg.data[0] |
                              ((uint32_t)msg.data[1] << 8) |
                              ((uint32_t)msg.data[2] << 16) |
                              ((uint32_t)msg.data[3] << 24);
            uint32_t provider = registry_lookup(svc_id);

            ipc_message_t reply = {0};
            reply.type    = MSG_SERVICE_REPLY;
            reply.target  = msg.source;
            reply.data[0] = provider & 0xFF;
            reply.data[1] = (provider >> 8)  & 0xFF;
            reply.data[2] = (provider >> 16) & 0xFF;
            reply.data[3] = (provider >> 24) & 0xFF;
            reply.size    = sizeof(uint32_t);
            _sys_ipc_send(msg.source, &reply);
            break;
        }
        default:
            /* Unknown message type — ignore */
            break;
        }
    }
}

/* ------------------------------------------------------------------ */
/* Kernel-side entry point — called once by kernel_main via           */
/* sched_create_thread(init_service_entry)                            */
/* ------------------------------------------------------------------ */

/*
 * A small user stack for init.  In a real system this would be a
 * proper page allocated by mm_alloc_page and mapped with MM_FLAG_USER_RW.
 * For now we place it in BSS; it is accessible from ring 3 because the
 * current page tables use a flat identity map with user bits set.
 */
static uint8_t init_user_stack[4096] __attribute__((aligned(16)));

void init_service_entry(void) {
    print("Init: starting\n", 0x0F);

    registry_init();

    print("Init: service registry ready\n", 0x0F);
    print("Init: dropping to ring 3\n", 0x0F);

    uint64_t stack_top = (uint64_t)(uintptr_t)(init_user_stack + sizeof(init_user_stack));
    drop_to_usermode((uint64_t)(uintptr_t)init_loop, stack_top);
}
