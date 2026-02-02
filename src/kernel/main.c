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


// Simple VGA text mode buffer
// VGA memory starts at 0xB8000 in 80x25 text mode
#define VGA_MEMORY ((volatile uint16_t*)0xB8000)

// Current cursor position for text output
static int cursor_x = 0;
static int cursor_y = 0;

// Ensure kernel_main is not name-mangled
#ifdef __cplusplus
extern "C" {
#endif

// Main kernel function - called from entry point
void kernel_main(void) {
    // Clear the screen with a blue background
    clear_screen(0x1F);

    // Print welcome message
    print("Welcome to E-comOS Kernel v0.0.1!\n", 0x1F);
    print("Initializing System...\n\n", 0x1F);

    // Phase 1: Early Initialization (inspired by Linux start_kernel)
    print("Phase 1: Early System Initialization\n", 0x1F);
    print("------------------------------------\n", 0x1F);
    
    // Initialize memory management first (like Linux's bootmem_init)
    print("Initializing memory management... ", 0x1F);
    mm_enable_paging();
    print("[OK] Paging enabled\n", 0x2F);

    // Phase 2: Core Subsystem Initialization
    print("\nPhase 2: Core Subsystem Tests\n", 0x1F);
    print("-----------------------------\n", 0x1F);
    
    // Test 1: Basic page allocation (like Linux's page allocator test)
    print("1. Testing page allocator... ", 0x1F);
    void* page1 = mm_alloc_page();
    void* page2 = mm_alloc_page();
    void* page3 = mm_alloc_page();
    
    if (page1 && page2 && page3) {
        print("[OK] Basic allocation working\n", 0x2F);
        
        // Test page deallocation and reuse
        mm_free_page(page2);
        void* page4 = mm_alloc_page();
        if (page4 == page2) {
            print("   [OK] Page reuse verified\n", 0x2F);
        }
    } else {
        print("[FAIL] Memory allocation failed\n", 0x4F);
        // In a real system, we might panic here like Linux does
    }

    // Test 2: Memory mapping functionality
    print("2. Testing memory mapping... ", 0x1F);
    if (mm_map_page(0x200000, (uint32_t)page3, PTE_PRESENT | PTE_WRITABLE) == 0) {
        print("[OK] Virtual memory mapping working\n", 0x2F);
    } else {
        print("[FAIL] Memory mapping failed\n", 0x4F);
    }

    // Test 3: Stress testing (like Linux's memory stress tests)
    print("3. Running memory stress test... ", 0x1F);
    #define STRESS_PAGES 10
    void* stress_pages[STRESS_PAGES];
    int allocated = 0;
    
    for (int i = 0; i < STRESS_PAGES; i++) {
        stress_pages[i] = mm_alloc_page();
        if (stress_pages[i]) {
            allocated++;
            // Write test pattern (like Linux's memory corruption checks)
            uint32_t* data = (uint32_t*)stress_pages[i];
            data[0] = 0xDEADBEEF;
            data[1] = i;
        }
    }
    
    // Verify data integrity
    int data_ok = 1;
    for (int i = 0; i < allocated; i++) {
        uint32_t* data = (uint32_t*)stress_pages[i];
        if (data[0] != 0xDEADBEEF || data[1] != i) {
            data_ok = 0;
            break;
        }
    }
    
    if (data_ok && allocated == STRESS_PAGES) {
        print("[OK] Stress test passed\n", 0x2F);
    } else {
        print("[FAIL] Data corruption detected\n", 0x4F);
    }
    
    // Cleanup
    for (int i = 0; i < allocated; i++) {
        mm_free_page(stress_pages[i]);
    }

    // Phase 3: System Statistics (like Linux's /proc/meminfo)
    print("\nPhase 3: System Statistics\n", 0x1F);
    print("---------------------------\n", 0x1F);
    
    // Calculate memory usage statistics
    uint32_t used_pages = 0;
    for (uint32_t i = 0; i < MAX_PAGES; i++) {
        uint32_t byte_idx = i / 8;
        uint32_t bit_idx = i % 8;
        if (page_bitmap[byte_idx] & (1 << bit_idx)) {
            used_pages++;
        }
    }
    
    print("Memory Usage:\n", 0x1F);
    print("  Total: 16MB (", 0x1F); print_number(MAX_PAGES, 0x1F); print(" pages)\n", 0x1F);
    print("  Used:  ", 0x1F); print_number(used_pages, 0x1F);
    print(" pages (", 0x1F); print_number((used_pages * 100) / MAX_PAGES, 0x1F); print("%)\n", 0x1F);
    print("  Free:  ", 0x1F); print_number(MAX_PAGES - used_pages, 0x1F);
    print(" pages (", 0x1F); print_number(((MAX_PAGES - used_pages) * 100) / MAX_PAGES, 0x1F); print("%)\n", 0x1F);

    // Phase 4: Process Simulation (like Linux's init process creation)
    print("\nPhase 4: Process Management Simulation\n", 0x1F);
    print("------------------------------------\n", 0x1F);
    
    // Simulate process creation (similar to Linux's fork infrastructure)
    print("Creating simulated processes... ", 0x1F);
    void* proc1_code = mm_alloc_page();
    void* proc1_stack = mm_alloc_page();
    void* proc2_code = mm_alloc_page();
    void* proc2_stack = mm_alloc_page();
    
    if (proc1_code && proc1_stack && proc2_code && proc2_stack) {
        print("[OK] Process memory allocated\n", 0x2F);
        
        // Initialize process data (like Linux's task_struct setup)
        uint32_t* proc1_data = (uint32_t*)proc1_code;
        uint32_t* proc2_data = (uint32_t*)proc2_code;
        proc1_data[0] = 0x12345678;  // Simulated process 1 data
        proc2_data[0] = 0x87654321;  // Simulated process 2 data
        
        // Cleanup (in real system, processes would continue running)
        mm_free_page(proc1_code);
        mm_free_page(proc1_stack);
        mm_free_page(proc2_code);
        mm_free_page(proc2_stack);
    } else {
        print("[FAIL] Process creation failed\n", 0x4F);
    }

    // Phase 5: System Ready (like Linux before rest_init())
    print("\nSystem Initialization Complete!\n", 0x2F);
    print("E-comOS Kernel v0.0.1 is now running.\n", 0x1F);
    print("Memory Manager: ACTIVE\n", 0x2F);
    print("Entering system idle loop...\n\n", 0x1F);

    // Main system loop (replaces the boring HLT-only approach)
    // This is similar to Linux's idle loop but more interactive for demonstration
    uint32_t system_ticks = 0;
    void* dynamic_pages[5] = {0};
    uint32_t alloc_cycle = 0;

    while (1) {
        system_ticks++;

        // Dynamic memory management demonstration
        if (system_ticks % 50 == 0) {
            // Allocate pages in a round-robin fashion
            if (!dynamic_pages[alloc_cycle % 5]) {
                dynamic_pages[alloc_cycle % 5] = mm_alloc_page();
                if (dynamic_pages[alloc_cycle % 5]) {
                    print("[MM] Allocated page at 0x", 0x2F);
                    print_hex((uint32_t)dynamic_pages[alloc_cycle % 5], 0x2F);
                    print("\n", 0x2F);
                }
            }
            alloc_cycle++;
        }

        // Periodic memory cleanup
        if (system_ticks % 123 == 0) {
            for (int i = 0; i < 5; i++) {
                if (dynamic_pages[i] && (system_ticks % (123 + i * 10) == 0)) {
                    print("[MM] Freed page at 0x", 0x6F);
                    print_hex((uint32_t)dynamic_pages[i], 0x6F);
                    print("\n", 0x6F);
                    mm_free_page(dynamic_pages[i]);
                    dynamic_pages[i] = 0;
                    break;
                }
            }
        }

        // System status display (like Linux's periodic statistics)
        if (system_ticks % 200 == 0) {
            uint32_t current_used = 0;
            for (uint32_t i = 0; i < MAX_PAGES; i++) {
                uint32_t byte_idx = i / 8;
                uint32_t bit_idx = i % 8;
                if (page_bitmap[byte_idx] & (1 << bit_idx)) current_used++;
            }
            
            print("[STATUS] System uptime: ", 0x1F);
            print_number(system_ticks, 0x1F);
            print(" | Memory: ", 0x1F);
            print_number(current_used, 0x1F);
            print("/", 0x1F);
            print_number(MAX_PAGES, 0x1F);
            print(" pages used\n", 0x1F);
        }

        // Brief delay instead of constant HLT
        // In a real system, this would be interrupt-driven
        for (volatile int delay = 0; delay < 50000; delay++);
        
        // Still use HLT but less frequently for power saving
        if (system_ticks % 10 == 0) {
            __asm__ volatile ("hlt");
        }
    }
}


#ifdef __cplusplus
}
#endif
