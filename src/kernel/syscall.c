/* 
    E-comOS Kernel - A Microkernel for E-comOS
    Copyright (C) 2025,2026 Saladin5101

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

#include <kernel/syscall.h>
#include <kernel/ipc.h>
#include <kernel/sched.h>
#include <kernel/mm.h>
#include <kernel/arch/interrupts.h>
#include <kernel/time.h>
#include <kernel/process.h>
#include <stdint.h>
#include <stddef.h>

#define MAX_IRQ_WAITERS 16
#define MAX_IRQS 16

// Structure to track processes waiting for IRQs
typedef struct {
    uint32_t pid;            // Process ID waiting for IRQ
    uint8_t irq_number;      // IRQ number to wait for
    uint8_t flags;           // Wait flags
    uint32_t timeout_ms;     // Timeout in milliseconds (0 = no timeout)
    uint64_t start_time;     // Time when wait started
    uint8_t is_active;       // Whether this waiter is active
} irq_waiter_t;

// Array of processes waiting for IRQs
static irq_waiter_t irq_waiters[MAX_IRQ_WAITERS];
static uint32_t num_waiters = 0;

// IRQ occurrence tracking
static volatile uint32_t irq_occurred[MAX_IRQS];
static volatile uint32_t irq_occurrence_count[MAX_IRQS];

// Initialize IRQ wait system
void syscall_irq_init(void) {
    for (int i = 0; i < MAX_IRQ_WAITERS; i++) {
        irq_waiters[i].pid = 0;
        irq_waiters[i].is_active = 0;
    }
    
    for (int i = 0; i < MAX_IRQS; i++) {
        irq_occurred[i] = 0;
        irq_occurrence_count[i] = 0;
    }
    
    num_waiters = 0;
}

// Notify that an IRQ has occurred
void syscall_irq_notify(uint8_t irq_num) {
    if (irq_num >= MAX_IRQS) {
        return; // Invalid IRQ number
    }
    
    // Mark IRQ as occurred
    irq_occurred[irq_num] = 1;
    irq_occurrence_count[irq_num]++;
    
    // Wake up all processes waiting for this IRQ
    for (uint32_t i = 0; i < num_waiters; i++) {
        if (irq_waiters[i].is_active && 
            irq_waiters[i].irq_number == irq_num && 
            irq_waiters[i].pid != 0) {
            
            // Clear the waiter
            irq_waiters[i].is_active = 0;
            
            // Wake up the process with PID 'pid'
            // This requires integration with the scheduler
            process_t* proc = sched_get_process_by_pid(irq_waiters[i].pid);
            if (proc != NULL && proc->state == PROCESS_STATE_BLOCKED) {
                proc->state = PROCESS_STATE_READY;
                proc->block_reason = BLOCK_REASON_NONE;
                // The scheduler will pick up this process on next schedule
            }
        }
    }
}

// Check if a process is waiting for a specific IRQ
static int find_irq_waiter(uint32_t pid, uint8_t irq_num) {
    for (uint32_t i = 0; i < num_waiters; i++) {
        if (irq_waiters[i].is_active && 
            irq_waiters[i].pid == pid && 
            irq_waiters[i].irq_number == irq_num) {
            return i;
        }
    }
    return -1;
}

// Add a process to the IRQ wait list
static int add_irq_waiter(uint32_t pid, uint8_t irq_num, uint8_t flags, uint32_t timeout_ms) {
    if (num_waiters >= MAX_IRQ_WAITERS) {
        return -1; // No space
    }
    
    // Check if already waiting
    if (find_irq_waiter(pid, irq_num) >= 0) {
        return -2; // Already waiting
    }
    
    // Find empty slot
    for (uint32_t i = 0; i < MAX_IRQ_WAITERS; i++) {
        if (!irq_waiters[i].is_active) {
            irq_waiters[i].pid = pid;
            irq_waiters[i].irq_number = irq_num;
            irq_waiters[i].flags = flags;
            irq_waiters[i].timeout_ms = timeout_ms;
            irq_waiters[i].start_time = time_get_current_ms(); // Get current time
            irq_waiters[i].is_active = 1;
            
            if (i >= num_waiters) {
                num_waiters = i + 1;
            }
            return 0;
        }
    }
    
    return -1;
}

// Remove a process from the IRQ wait list
static int remove_irq_waiter(uint32_t pid, uint8_t irq_num) {
    int index = find_irq_waiter(pid, irq_num);
    if (index < 0) {
        return -1; // Not found
    }
    
    irq_waiters[index].is_active = 0;
    return 0;
}

// Check for timed out waiters
void syscall_irq_check_timeouts(void) {
    // Implement timeout checking
    // This requires a system timer
    uint64_t current_time = time_get_current_ms();
    
    for (uint32_t i = 0; i < num_waiters; i++) {
        if (irq_waiters[i].is_active && irq_waiters[i].pid != 0) {
            // For each waiter with timeout > 0, check if time has elapsed
            if (irq_waiters[i].timeout_ms > 0) {
                uint64_t elapsed = current_time - irq_waiters[i].start_time;
                if (elapsed >= irq_waiters[i].timeout_ms) {
                    // If timeout reached, remove waiter and wake process with timeout error
                    uint32_t pid = irq_waiters[i].pid;
                    irq_waiters[i].is_active = 0;
                    
                    process_t* proc = sched_get_process_by_pid(pid);
                    if (proc != NULL && proc->state == PROCESS_STATE_BLOCKED) {
                        proc->state = PROCESS_STATE_READY;
                        proc->block_reason = BLOCK_REASON_NONE;
                        // Set error code for timeout
                        proc->last_error = ERR_TIMEOUT;
                    }
                }
            }
        }
    }
}

// IRQ wait system call implementation
long syscall_irq_wait(uint8_t irq_num, uint8_t flags, uint32_t timeout_ms) {
    // Validate IRQ number
    if (irq_num >= MAX_IRQS) {
        return -1; // Invalid IRQ number
    }
    
    // Get current process ID from scheduler
    uint32_t current_pid = sched_get_current_pid();
    
    // Add error handling for invalid process ID
    if (current_pid == 0) {
        return -4; // Invalid process ID
    }
    
    // Check if IRQ has already occurred
    if (irq_occurred[irq_num]) {
        if (flags & IRQ_WAIT_CLEAR) {
            irq_occurred[irq_num] = 0; // Clear occurrence
        }
        return 0; // Success - IRQ already occurred
    }
    
    // Check for immediate return flag
    if (flags & IRQ_WAIT_NOWAIT) {
        return -2; // Would block, but NOWAIT flag is set
    }
    
    // Add process to wait list
    int result = add_irq_waiter(current_pid, irq_num, flags, timeout_ms);
    if (result < 0) {
        return result; // Error adding waiter
    }
    
    // Block the current process until IRQ occurs or timeout
    // This requires scheduler integration
    process_t* current_proc = sched_get_current_process();
    if (current_proc == NULL) {
        remove_irq_waiter(current_pid, irq_num);
        return -4; // Invalid process
    }
    
    // Set process state to blocked (waiting for IRQ)
    current_proc->state = PROCESS_STATE_BLOCKED;
    current_proc->block_reason = BLOCK_REASON_IRQ_WAIT;
    current_proc->block_data.irq_num = irq_num;
    
    // In a real implementation, the scheduler would put the process to sleep
    // For now, we implement a simple blocking mechanism
    while (!irq_occurred[irq_num]) {
        // Check for timeout
        syscall_irq_check_timeouts();
        
        // Ensure `find_irq_waiter` return value is properly checked
        if (find_irq_waiter(current_pid, irq_num) < 0) {
            // Waiter was removed (likely due to timeout)
            break;
        }
        
        // Yield to other processes
        sched_yield();
    }
    
    // Clear the IRQ occurrence if needed
    if (irq_occurred[irq_num] && (flags & IRQ_WAIT_CLEAR)) {
        irq_occurred[irq_num] = 0;
    }
    
    // Remove from wait list
    remove_irq_waiter(current_pid, irq_num);
    
    // Check if we woke up due to timeout
    if (current_proc->last_error == ERR_TIMEOUT) {
        current_proc->last_error = 0; // Clear error
        return -3; // Timeout error
    }
    
    return 0; // Success
}

// Get IRQ occurrence count
long syscall_irq_get_count(uint8_t irq_num) {
    if (irq_num >= MAX_IRQS) {
        return -1; // Invalid IRQ number
    }
    
    return irq_occurrence_count[irq_num];
}

// Reset IRQ occurrence count
long syscall_irq_reset_count(uint8_t irq_num) {
    if (irq_num >= MAX_IRQS) {
        return -1; // Invalid IRQ number
    }
    
    uint32_t old_count = irq_occurrence_count[irq_num];
    irq_occurrence_count[irq_num] = 0;
    return old_count;
}

// Main system call handler
long syscall_handler(uint32_t syscall_num, uint32_t arg1, uint32_t arg2, uint32_t arg3) {
    switch (syscall_num) {
        case SYS_IPC_SEND:
            return ipc_send((thread_id_t)arg1, (struct ipc_message*)arg2);
            
        case SYS_IPC_RECEIVE:
            return ipc_receive((struct ipc_message*)arg1);
            
        case SYS_THREAD_YIELD:
            sched_yield();
            return 0;
            
        case SYS_ADDRESS_MAP:
            return mm_map_page(arg1, arg2, arg3);
            
        case SYS_IRQ_WAIT:
            return syscall_irq_wait((uint8_t)arg1, (uint8_t)arg2, arg3);
            
        case SYS_IRQ_GET_COUNT:
            return syscall_irq_get_count((uint8_t)arg1);
            
        case SYS_IRQ_RESET_COUNT:
            return syscall_irq_reset_count((uint8_t)arg1);
            
        default:
            return -1; // Invalid syscall
    }
}

// Suppress unused parameter warning for `color`
void clear_screen(uint8_t color) {
    (void)color; // Explicitly mark as unused
    // ...existing code...
}