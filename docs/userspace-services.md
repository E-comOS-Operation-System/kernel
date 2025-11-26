# E-comOS Userspace Services Architecture

## Microkernel (< 32KB)
```c
// ONLY these 4 components in kernel:
1. Address space switching (CR3, privileged)
2. Thread scheduler (minimal round-robin)  
3. IPC primitives (cross-address-space)
4. Interrupt dispatch (IDT, privileged)
```

## Userspace Services (Everything Else)

### Core System Services
```c
1. Object Manager Service     // Handles object creation/destruction
2. Process Manager Service    // Process lifecycle management  
3. Memory Manager Service     // Heap allocation, memory pools
4. Service Registry Service   // Service discovery and naming
```

### Hardware Services  
```c
5. VGA Display Service       // Text/graphics output
6. Keyboard Input Service    // Keyboard input handling
7. Timer Service            // System time, alarms
8. Serial Port Service      // Debug/communication
```

### System Services
```c
9. File System Service      // File operations
10. Network Service         // TCP/IP stack
11. Shell Service          // Command interpreter
```

## Service Communication
```c
// All services communicate via IPC
// Example: App wants to display text
app_process -> IPC_SEND(vga_service, "Hello World")
vga_service -> receives message -> writes to VGA buffer
```

## Benefits
- **Security**: Services isolated in separate address spaces
- **Reliability**: Service crash doesn't crash kernel
- **Modularity**: Services can be updated independently
- **Performance**: Only essential operations require kernel mode switch