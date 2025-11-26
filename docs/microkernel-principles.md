# E-comOS Microkernel Design Principles

## What MUST be in the microkernel:
1. **Address space management** - Virtual memory, paging
2. **Thread management** - Basic scheduling, context switching  
3. **IPC** - Inter-process communication primitives
4. **Interrupt handling** - Hardware interrupt dispatch
5. **Capability system** - Security and access control

## What MUST NOT be in the microkernel:
1. **Device drivers** - All in userspace (VGA, keyboard, etc.)
2. **File systems** - Userspace services
3. **Network stack** - Userspace services
4. **Complex scheduling policies** - Basic round-robin only
5. **System services** - All via IPC to userspace servers

## Current Architecture Issues:
- ✅ GDT/IDT setup (required for x86)
- ✅ Basic paging (address space management)
- ✅ Context switching (thread management)
- ❌ Too much VGA code in kernel
- ❌ Missing capability system
- ❌ IPC not implemented

## Microkernel Size Target:
- **< 10,000 lines of code**
- **< 64KB binary size**
- **Minimal system calls** (~10-15 syscalls max)