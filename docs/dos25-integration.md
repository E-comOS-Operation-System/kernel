# DOS25 Bootloader Integration

## E-comOS Boot Chain
```
DOS25 Bootloader → E-comOS Microkernel → Userspace Services
```

## DOS25 Requirements
- DOS25 should load E-comOS kernel at 0x100000 (1MB)
- Pass control to kernel entry point `_start` in boot/boot.s
- Provide multiboot-compatible information structure

## Memory Layout
```
0x00000000 - 0x000FFFFF  DOS25 + Real mode area
0x00100000 - 0x0010FFFF  E-comOS Microkernel (64KB max)
0x00110000+              Userspace services
```

## Integration Steps
1. DOS25 loads kernel binary to 1MB
2. DOS25 sets up basic GDT/protected mode
3. DOS25 jumps to kernel `_start` function
4. Kernel takes over and starts userspace services

## Build Process
```bash
# Step 1: Build E-comOS kernel
make image

# Step 2: DOS25 integration (external)
# DOS25 build system will:
# - Take ecomos-kernel.bin
# - Embed it in DOS25 image
# - Create final bootable image
```

## Files for DOS25
- `ecomos-kernel.bin` - Raw kernel binary
- `arch/x86_64/boot/linker.ld` - Memory layout info
- Boot entry point at `_start` symbol