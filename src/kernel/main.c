/*
    E-comOS Kernel - A Microkernel for E-comOS
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
#include <kernel/arch/x86_64.h>
#include <kernel/arch/interrupts.h>
#include <kernel/mm.h>
#include <kernel/sched.h>
#include <kernel/ipc.h>
#include <kernel/syscall.h>
#include <kernel/printkit/print.h>

extern void syscall_irq_check_timeouts(void);
extern void syscall_irq_init(void);
extern void gdt_init(void);


// Simple VGA text mode buffer
// VGA memory starts at 0xB8000 in 80x25 text mode
#define VGA_MEMORY ((volatile uint16_t*)0xB8000)

// UEFI 入口
#include <efi/efi.h>
#include <efi/efilib.h>
#include <kernel/arch/x86_64.h>
#include <kernel/arch/interrupts.h>
#include <kernel/mm.h>
#include <kernel/sched.h>
#include <kernel/ipc.h>
#include <kernel/syscall.h>

extern void syscall_irq_check_timeouts(void);
extern void syscall_irq_init(void);
extern void gdt_init(void);

EFI_STATUS EFIAPI kernel_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
        extern uint32_t next_free_page;
    InitializeLib(ImageHandle, SystemTable);
    Print(L"E-comOS Microkernel v0.0.1 UEFI boot\n");
    Print(L"Initializing kernel...\n");

    // Phase 1b: GDT + TSS (must be before interrupts)
    Print(L"Setting up GDT + TSS...\n");
    gdt_init();

    // Phase 2: Memory subsystem
    Print(L"Initializing memory subsystem...\n");
    mm_enable_paging();

    // Phase 3: Interrupt and exception handling
    Print(L"Setting up interrupt handling...\n");
    idt_init();
    irq_remap();
    irq_init_timer();

    // Phase 4: Scheduler initialization
    Print(L"Initializing scheduler...\n");
    // Scheduler already initialized via static arrays

    // Phase 5: IPC + syscall IRQ init
    Print(L"Initializing IPC subsystem...\n");
    syscall_irq_init();

    // Phase 6: Create initial thread (init service)
    Print(L"Creating initial thread (init service)...\n");
    extern void init_service_entry(void);
    int init_tid = sched_create_thread(init_service_entry);
    if (init_tid > 0) {
        Print(L"Init service created with TID: %d\n", init_tid);
    }

    // Phase 7: Enable interrupts and enter kernel loop
    Print(L"Kernel initialization complete. Starting scheduler...\n");
    __asm__ volatile ("sti");

    while (1) {
        sched_schedule();
        ipc_message_t pending_msg;
        if (ipc_receive_msg(&pending_msg, 0) == ECLIB_OK) {
            ipc_send((thread_id_t)pending_msg.target, &pending_msg);
        }
        syscall_irq_check_timeouts();
        static uint32_t mm_check_counter = 0;
        if (++mm_check_counter % 100 == 0) {
            uint32_t used_pages = 0;
            for (uint32_t i = 0; i < MAX_PAGES; i++) {
                uint32_t byte_idx = i / 8;
                uint32_t bit_idx  = i % 8;
                if (page_bitmap[byte_idx] & (1u << bit_idx)) used_pages++;
            }
            if (used_pages > (MAX_PAGES * 80 / 100)) {
                next_free_page = 0;
            }
        }
        __asm__ volatile ("hlt");
    }
    return EFI_SUCCESS;
}
