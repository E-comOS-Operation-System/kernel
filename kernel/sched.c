/*  E-comOS - A Microkernel-based Operating System
*   Copyright (C) 2025  Saladin5101
*
*   This program is free software: you can redistribute it and/or modify
*   it under the terms of the GNU Affero General Public License as published
*   by the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU Affero General Public License for more details.
*
*   You should have received a copy of the GNU Affero General Public License
*   along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
// kernel/sched.c - Task scheduling for E-comOS microkernel
// Manages user-space tasks and basic round-robin scheduling
#include "kernel.h"
#include <stddef.h>

// Task state definitions
#define TASK_RUNNING 0
#define TASK_READY   1
#define TASK_BLOCKED 2

// Task structure (stores task context and state)
typedef struct task {
    uint64_t rsp;         
    uint64_t rbp;          
    uint64_t pid;            
    int state;
    struct task* next;     
    page_table_entry_t* pml4; 
} task_t;

// Scheduler queue (linked list of ready tasks)
static task_t* sched_queue = NULL;
// Current running task
static task_t* current_task = NULL;
// Next available PID
static uint64_t next_pid = 1;

// Helper: Allocate a stack for user-space task (4KB stack)
static void* sched_alloc_task_stack() {
    void* phys_stack = mm_alloc_page();
    if (!phys_stack) {
        return NULL;
    }
    // User stack is in low virtual address (0x1000000 ~ 0x2000000)
    static uint64_t user_stack_virt = 0x1000000;
    void* virt_stack = (void*)user_stack_virt;
    
    // Map stack to user-accessible virtual address
    mm_map_page(virt_stack, phys_stack, 1); // 1 = user-space allowed
    user_stack_virt += PAGE_SIZE; // Next stack for next task
    
    // Stack grows downward: return top of stack (align to 8 bytes)
    return (void*)(user_stack_virt - 8);
}

// Create a new task (user-space or kernel-space)
// Parameters:
//   entry: Task entry point (function address, virtual address)
//   is_user: 1 = user-space task; 0 = kernel-space task
// Return: PID of new task (0 on failure)
uint64_t sched_create_task(void (*entry)(), int is_user) {
    // 1. Allocate task structure
    task_t* new_task = (task_t*)mm_alloc_page();
    if (!new_task) {
        return 0;
    }
    new_task->pid = next_pid++;
    new_task->state = TASK_READY;
    new_task->next = NULL;
    
    // 2. Create task-specific page table (copy kernel mapping + add user mapping)
    new_task->pml4 = mm_create_table();
    if (!new_task->pml4) {
        return 0;
    }
    // Copy kernel PML4 entry (so task can access kernel via virtual address)
    new_task->pml4[511] = kernel_pml4[511]; // PML4 index 511 = kernel virtual address
    
    // 3. Allocate and initialize task stack
    new_task->rsp = (uint64_t)sched_alloc_task_stack();
    if (!new_task->rsp) {
        return 0;
    }
    
    // 4. Set up stack for context switch (x86_64 calling convention)
    // Push dummy RFLAGS (enable interrupts: bit 9 = IF)
    new_task->rsp -= 8;
    *(uint64_t*)new_task->rsp = 0x202; // IF = 1, IOPL = 0
    // Push dummy CS (code segment selector: user = 0x1B, kernel = 0x08)
    new_task->rsp -= 8;
    *(uint64_t*)new_task->rsp = is_user ? 0x1B : 0x08;
    // Push task entry point (RIP)
    new_task->rsp -= 8;
    *(uint64_t*)new_task->rsp = (uint64_t)entry;
    // Push dummy registers (RBP, RDI-R15) to fill stack frame
    for (int i = 0; i < 13; i++) {
        new_task->rsp -= 8;
        *(uint64_t*)new_task->rsp = 0;
    }
    
    // 5. Add task to scheduler queue
    if (!sched_queue) {
        sched_queue = new_task;
    } else {
        task_t* temp = sched_queue;
        while (temp->next) {
            temp = temp->next;
        }
        temp->next = new_task;
    }
    
    vga_put_char(0, 3, 'T', VGA_COLOR); // Show 'Task X Created'
    vga_put_char(1, 3, 'a', VGA_COLOR);
    vga_put_char(2, 3, 's', VGA_COLOR);
    vga_put_char(3, 3, 'k', VGA_COLOR);
    vga_put_char(4, 3, ' ', VGA_COLOR);
    vga_put_char(5, 3, '0' + (new_task->pid % 10), VGA_COLOR); // Show PID (0-9)
    vga_put_char(6, 3, ' ', VGA_COLOR);
    vga_put_char(7, 3, 'C', VGA_COLOR);
    vga_put_char(8, 3, 'r', VGA_COLOR);
    vga_put_char(9, 3, 'e', VGA_COLOR);
    vga_put_char(10, 3, 'a', VGA_COLOR);
    vga_put_char(11, 3, 't', VGA_COLOR);
    vga_put_char(12, 3, 'e', VGA_COLOR);
    
    return new_task->pid;
}

// Switch to next task in scheduler queue (round-robin)
// Called via timer interrupt (simplified: manual call for initial version)
void sched_switch_task() {
    if (!sched_queue || !current_task) {
        return;
    }
    
    // 1. Save current task's context (RBP, RSP)
    asm volatile ("mov %%rsp, %0; mov %%rbp, %1" 
                 : "=m"(current_task->rsp), "=m"(current_task->rbp)
                 : : "memory");
    
    // 2. Move current task to end of queue (round-robin)
    task_t* prev_task = current_task;
    current_task = current_task->next;
    if (!current_task) {
        current_task = sched_queue; // Wrap to start
    }
    // Update queue (remove prev task from front, add to end)
    sched_queue = current_task;
    task_t* temp = current_task;
    while (temp->next) {
        temp = temp->next;
    }
    temp->next = prev_task;
    prev_task->next = NULL;
    prev_task->state = TASK_READY;
    current_task->state = TASK_RUNNING;
    
    // 3. Load next task's page table (CR3) and context
    uint64_t new_pml4_phys = (uint64_t)current_task->pml4 - KERNEL_VIRT_BASE;
    asm volatile (
        "mov %0, %%cr3\n"          // Load new page table
        "mov %1, %%rbp\n"          // Restore RBP
        "mov %2, %%rsp\n"          // Restore RSP
        "iretq"                    // Restore RIP/CS/RFLAGS (user-space)
        : : "r"(new_pml4_phys), "m"(current_task->rbp), "m"(current_task->rsp)
        : "memory"
    );
}

// Initialize scheduler (set up first kernel task)
void sched_init() {
    // Create a dummy kernel task (current running context)
    current_task = (task_t*)mm_alloc_page();
    if (!current_task) {
        vga_put_char(0, 4, 'S', 0x04); // Red 'S' for scheduler init fail
        vga_put_char(1, 4, 'c', 0x04);
        vga_put_char(2, 4, 'h', 0x04);
        vga_put_char(3, 4, 'e', 0x04);
        vga_put_char(4, 4, 'd', 0x04);
        vga_put_char(5, 4, ' ', 0x04);
        vga_put_char(6, 4, 'F', 0x04);
        vga_put_char(7, 4, 'a', 0x04);
        vga_put_char(8, 4, 'i', 0x04);
        vga_put_char(9, 4, 'l', 0x04);
        while (1);
    }
    current_task->pid = 0; // PID 0 = kernel idle task
    current_task->state = TASK_RUNNING;
    current_task->pml4 = kernel_pml4;
    current_task->next = NULL;
    
    // Save initial context (RBP, RSP)
    asm volatile ("mov %%rsp, %0; mov %%rbp, %1" 
                 : "=m"(current_task->rsp), "=m"(current_task->rbp)
                 : : "memory");
    
    vga_put_char(0, 4, 'S', VGA_COLOR); // Show 'Sched Init OK'
    vga_put_char(1, 4, 'c', VGA_COLOR);
    vga_put_char(2, 4, 'h', VGA_COLOR);
    vga_put_char(3, 4, 'e', VGA_COLOR);
    vga_put_char(4, 4, 'd', VGA_COLOR);
    vga_put_char(5, 4, ' ', VGA_COLOR);
    vga_put_char(6, 4, 'I', VGA_COLOR);
    vga_put_char(7, 4, 'n', VGA_COLOR);
    vga_put_char(8, 4, 'i', VGA_COLOR);
    vga_put_char(9, 4, 't', VGA_COLOR);
    vga_put_char(10, 4, ' ', VGA_COLOR);
    vga_put_char(11, 4, 'O', VGA_COLOR);
    vga_put_char(12, 4, 'K', VGA_COLOR);
}