/*
    E-comOS Init Service - First userspace process (PID 1)
    Copyright (C) 2025,2026  Saladin5101

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as published
    by the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <stdint.h>
#include <kernel/ipc.h>
#include <kernel/syscall.h>
#include <kernel/printkit/print.h>

#define MSG_SERVICE_REGISTER 0x01
#define MSG_SERVICE_LOOKUP   0x02
#define MSG_SERVICE_REPLY    0x03
#define MAX_SERVICES         32

typedef struct {
    uint32_t serviceId;
    uint32_t providerPid;
    uint8_t  active;
} ServiceEntry;

static ServiceEntry serviceTable[MAX_SERVICES];

static void registryInit(void) {
    for (int i = 0; i < MAX_SERVICES; i++)
        serviceTable[i].active = 0;
}

static int registryRegister(uint32_t serviceId, uint32_t providerPid) {
    for (int i = 0; i < MAX_SERVICES; i++) {
        if (!serviceTable[i].active) {
            serviceTable[i].serviceId   = serviceId;
            serviceTable[i].providerPid = providerPid;
            serviceTable[i].active      = 1;
            return 0;
        }
    }
    return -1;
}

static uint32_t registryLookup(uint32_t serviceId) {
    for (int i = 0; i < MAX_SERVICES; i++) {
        if (serviceTable[i].active && serviceTable[i].serviceId == serviceId)
            return serviceTable[i].providerPid;
    }
    return 0;
}

static void __attribute__((noreturn))
dropToUsermode(uint64_t entryPoint, uint64_t userStackTop) {
    __asm__ volatile(
        "cli\n"
        "movw $0x23, %%ax\n"
        "movw %%ax, %%ds\n"
        "movw %%ax, %%es\n"
        "movw %%ax, %%fs\n"
        "movw %%ax, %%gs\n"
        "pushq $0x23\n"
        "pushq %[sp]\n"
        "pushfq\n"
        "orq  $0x200, (%%rsp)\n"
        "pushq $0x1B\n"
        "pushq %[ip]\n"
        "iretq\n"
        :
        : [ip] "r"(entryPoint), [sp] "r"(userStackTop)
        : "rax", "memory"
    );
    __builtin_unreachable();
}

static inline long sysIpcSend(uint32_t target, IpcMessage *msg) {
    long ret;
    __asm__ volatile("int $0x80"
        : "=a"(ret)
        : "a"(SYS_IPC_SEND), "b"(target),
          "c"((uint32_t)(uintptr_t)msg), "d"(0)
        : "memory");
    return ret;
}

static inline long sysIpcReceive(IpcMessage *msg) {
    long ret;
    __asm__ volatile("int $0x80"
        : "=a"(ret)
        : "a"(SYS_IPC_RECEIVE), "b"((uint32_t)(uintptr_t)msg),
          "c"(0), "d"(0)
        : "memory");
    return ret;
}

static inline void sysYield(void) {
    __asm__ volatile("int $0x80"
        : : "a"(SYS_THREAD_YIELD), "b"(0), "c"(0), "d"(0)
        : "memory");
}

static void __attribute__((noreturn)) initLoop(void) {
    IpcMessage msg;
    while (1) {
        long rc = sysIpcReceive(&msg);
        if (rc != ECLIB_OK) {
            sysYield();
            continue;
        }
        switch (msg.type) {
        case MSG_SERVICE_REGISTER: {
            uint32_t svcId = msg.data[0]
                | ((uint32_t)msg.data[1] << 8)
                | ((uint32_t)msg.data[2] << 16)
                | ((uint32_t)msg.data[3] << 24);
            registryRegister(svcId, msg.source);
            break;
        }
        case MSG_SERVICE_LOOKUP: {
            uint32_t svcId = msg.data[0]
                | ((uint32_t)msg.data[1] << 8)
                | ((uint32_t)msg.data[2] << 16)
                | ((uint32_t)msg.data[3] << 24);
            uint32_t provider = registryLookup(svcId);
            IpcMessage reply  = {0};
            reply.type        = MSG_SERVICE_REPLY;
            reply.target      = msg.source;
            reply.size        = sizeof(uint32_t);
            reply.data[0]     = provider & 0xFF;
            reply.data[1]     = (provider >> 8)  & 0xFF;
            reply.data[2]     = (provider >> 16) & 0xFF;
            reply.data[3]     = (provider >> 24) & 0xFF;
            sysIpcSend(msg.source, &reply);
            break;
        }
        default:
            break;
        }
    }
}

static uint8_t initUserStack[4096] __attribute__((aligned(16)));

void initServiceEntry(void) {
    printStr("Init: starting\n", 0x0F);
    registryInit();
    printStr("Init: registry ready\n", 0x0F);
    printStr("Init: dropping to ring 3\n", 0x0F);
    uint64_t stackTop = (uint64_t)(uintptr_t)(initUserStack + sizeof(initUserStack));
    dropToUsermode((uint64_t)(uintptr_t)initLoop, stackTop);
}
