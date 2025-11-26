# E-comOS TODO List

## Memory Management (mm/)
- [ ] Implement proper page table management (src/mm/mm.c)

## Kernel Core (kernel/)
- [ ] Display panic message (src/kernel/panic.c)
- [ ] Implement kernel logging (src/kernel/panic.c)
- [ ] Implement IRQ waiting (src/kernel/syscall.c)
- [ ] Start userspace services (VGA driver, etc.) (src/kernel/main.c)

## Architecture (arch/x86_64/)
- [ ] Complete interrupt handler setup
- [ ] Implement GDT management
- [ ] Add paging support

## Build System
- [ ] Fix cross-compiler detection
- [ ] Implement distro-base generation script
- [ ] Add proper boot sector

## Testing & Debug
- [ ] Add QEMU debugging support
- [ ] Implement kernel unit tests
- [ ] Add memory leak detection