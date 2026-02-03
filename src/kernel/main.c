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
#include <kernel/mm.h>
#include <kernel/sched.h>
#include <kernel/ipc.h>
#include <kernel/syscall.h>
#include <kernel/printkit/print.h>
#include <kernel/arch/interrupts.h>


// Simple VGA text mode buffer
// VGA memory starts at 0xB8000 in 80x25 text mode
#define VGA_MEMORY ((volatile uint16_t*)0xB8000)

// Ensure kernel_main is not name-mangled
#ifdef __cplusplus
extern "C" {
#endif

// Main kernel function - seL4 style initialization
void kernel_main(void) {
    // Phase 1: Early platform initialization (like seL4's init_kernel)
    clear_screen(0x1F);
    print("E-comOS Microkernel v0.0.1\n", 0x1F);
    print("Initializing kernel...\n", 0x1F);
    
    // Phase 2: Memory subsystem (seL4's init_freemem)
    print("Initializing memory subsystem...\n", 0x1F);
    mm_enable_paging();
    
    // Phase 3: Interrupt and exception handling (seL4's init_irqs)
    print("Setting up interrupt handling...\n", 0x1F);
    idt_init();
    irq_remap();
    
    // Phase 4: Scheduler initialization (seL4's init_sched)
    print("Initializing scheduler...\n", 0x1F);
    // Scheduler already initialized via static arrays
    
    // Phase 5: IPC initialization (seL4's init_ipc)
    print("Initializing IPC subsystem...\n", 0x1F);
    // syscall_irq_init(); // TODO: implement this function
    
    // Phase 6: Create initial thread (seL4's create_initial_thread)
    print("Creating initial thread (init service)...\n", 0x1F);
    extern void init_service_entry(void);
    int init_tid = sched_create_thread(init_service_entry);
    if (init_tid > 0) {
        print("Init service created with TID: ", 0x2F);
        print_number(init_tid, 0x2F);
        print("\n", 0x2F);
    }
    
    // Phase 7: Enable interrupts and enter kernel loop (seL4's schedule)
    print("Kernel initialization complete. Starting scheduler...\n", 0x2F);
    __asm__ volatile ("sti"); // Enable interrupts
    
    // seL4-style kernel loop: pure microkernel scheduler
    while (1) {
        // 1. Schedule next thread
        sched_schedule();
        
        // 2. Handle any pending kernel operations
        // Process pending IPC messages
        ipc_message_t pending_msg;
        if (ipc_receive(&pending_msg) == ECLIB_OK) {
            // Route message to target thread (simplified for now)
            // TODO: implement proper message routing
        }
        
        // Handle pending system calls
        // syscall_irq_check_timeouts();
        
        // Process memory management requests
        static uint32_t mm_check_counter = 0;
        if (++mm_check_counter % 100 == 0) {
            // Check for memory pressure and free unused pages
            uint32_t used_pages = 0;
            for (uint32_t i = 0; i < MAX_PAGES; i++) {
                uint32_t byte_idx = i / 8;
                uint32_t bit_idx = i % 8;
                if (page_bitmap[byte_idx] & (1 << bit_idx)) used_pages++;
            }
            // If memory usage > 80%, trigger cleanup
            if (used_pages > (MAX_PAGES * 80 / 100)) {
                // Simple memory pressure indication
                // TODO: implement proper memory reclamation
            }
        }
        
        // 3. Idle until interrupt
        __asm__ volatile ("hlt");
        
        // 4. When interrupt occurs, process it
        // Timer interrupt -> reschedule
        // System call interrupt -> handle syscall
        // Hardware interrupt -> handle device
    }
}


#ifdef __cplusplus
}
#endif
