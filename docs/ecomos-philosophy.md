# E-comOS Three Core Philosophies

## 1. Everything is a Service (万物皆服务)
- **VGA Display** → VGA Service Process
- **Keyboard Input** → Input Service Process  
- **File System** → FS Service Process
- **Network** → Network Service Process
- **Memory Management** → MM Service Process

## 2. Everything is an Object (万物皆对象)
- **Threads** → Thread Objects with capabilities
- **Memory Pages** → Memory Objects with access rights
- **IPC Channels** → Channel Objects with permissions
- **Hardware Resources** → Resource Objects with capabilities

## 3. Everything is a Process (万物皆进程)
- **Device Drivers** → Driver Processes
- **System Services** → Service Processes
- **Applications** → User Processes
- **Kernel Modules** → Kernel Processes (minimal)

## Implementation Strategy
```
Microkernel (< 64KB)
├── Object Manager (capability-based)
├── Process Manager (minimal scheduler)  
├── IPC Manager (message passing)
└── Memory Manager (address spaces)

Userspace Services (Everything Else)
├── VGA Service Process
├── Keyboard Service Process
├── FS Service Process
└── Shell Process
```