/*
    E-comOS Kernel - Main entry point
    Copyright (C) 2025,2026  Saladin5101

    Precondition:  called from _start with interrupts disabled.
    Precondition:  bootParams is either NULL or a valid UEFI memory map.
    Postcondition: never returns.
*/

#include <stdint.h>
#include <kernel/boot.h>
#include <kernel/arch/interrupts.h>
#include <kernel/mm.h>
#include <kernel/sched.h>
#include <kernel/ipc.h>
#include <kernel/syscall.h>
#include <kernel/printkit/print.h>
#include <kernel/debug.h>

extern void gdtInit(void);
extern void initServiceEntry(void);

void kernelMain(BootParams *bootParams) {
    /* Interrupts are disabled on entry from _start */

    clearScreen(0x1F);
    printStr("E-comOS Microkernel v0.0.1\n", 0x1F);
    printStr("Initializing kernel...\n", 0x1F);

    /* Phase 1: GDT + TSS — must be first; fixes segment registers */
    printStr("GDT + TSS...\n", 0x1F);
    gdtInit();

    /* Phase 2: Memory — must be before any mmAllocPage call */
    printStr("Memory subsystem...\n", 0x1F);
    MemoryStatus mmStatus = mmInit(bootParams);
    if (mmStatus != MEMORY_SUCCESS) {
        kernelPanic("mmInit failed — no usable memory");
    }
    mmEnablePaging();

    /* Phase 3: Interrupts */
    printStr("Interrupt handling...\n", 0x1F);
    idtInit();
    irqRemap();
    irqInitTimer();

    /* Phase 4: IPC + syscall IRQ subsystem */
    printStr("IPC + syscall...\n", 0x1F);
    syscallIrqInit();

    /* Phase 5: Create init service thread */
    printStr("Creating init service...\n", 0x1F);
    int initTid = schedCreateThread(initServiceEntry);
    if (initTid <= 0)
        kernelPanic("failed to create init service thread");

    printStr("Init TID: ", 0x2F);
    printNum((uint32_t)initTid, 0x2F);
    printStr("\n", 0x2F);

    printStr("Kernel ready.\n", 0x2F);

    /* Enable interrupts — from this point shared state must be protected */
    __asm__ volatile("sti");

    /* Kernel idle loop */
    while (1) {
        schedSchedule();

        /* Route any pending IPC messages.
         * Disable interrupts around bitmap/queue access to prevent
         * data races with IRQ handlers (F-09). */
        __asm__ volatile("cli");

        IpcMessage msg = {0};   /* zero-initialise to avoid garbage (F-11) */
        if (ipcReceiveMsg(&msg, 0) == ECLIB_OK)
            ipcSend((ThreadId)msg.target, &msg);

        syscallIrqCheckTimeouts();

        /* Memory pressure: reset allocator scan hint if >80% used */
        static uint32_t mmCounter = 0;
        if (++mmCounter % 100u == 0u) {
            uint32_t used = 0;
            for (uint32_t i = 0; i < MAX_PAGES; i++)
                if (pageBitmap[i >> 3] & (1u << (i & 7u)))
                    used++;
            if (used > (MAX_PAGES * 80u / 100u))
                nextFreePage = 0;
        }

        __asm__ volatile("sti");

        /* Halt until next interrupt */
        __asm__ volatile("hlt");
    }
}
