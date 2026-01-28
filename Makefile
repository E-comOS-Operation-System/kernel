#  	E-comOS Kernel - A Microkernel for E-comOS
#   Copyright (C) 2025,2026,2026  Saladin5101
#
#   This program is free software: you can redistribute it and/or modify
#   it under the terms of the GNU Affero General Public License as published
#   by the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU Affero General Public License for more details.
#
#   You should have received a copy of the GNU Affero General Public License
#   along with this program.  If not, see <https://www.gnu.org/licenses/>.

# Core targets:
# - make all      → Build everything (kernel + UEFI image)
# - make kernel   → Build kernel binary only  
# - make uefi     → Create UEFI bootable image
# - make run      → Test in QEMU with OVMF
# - make clean    → Clean all build artifacts

# =============================================================================
# CONFIGURATION
# =============================================================================

# Toolchain for UEFI 64-bit
CC := x86_64-elf-gcc
LD := x86_64-elf-ld
OBJCOPY := x86_64-elf-objcopy
NASM := nasm
QEMU := qemu-system-x86_64

# UEFI-specific flags
CFLAGS := -m64 -ffreestanding -fno-builtin -fno-stack-protector \
          -nostdlib -Wall -Wextra -c -mcmodel=large -mno-red-zone
LDFLAGS := -m elf_x86_64 -nostdlib -T uefi.ld

# Add include directory to compiler flags
CFLAGS += -Iinclude
# Add UEFI include path
CFLAGS += -I/usr/include/efi -I/usr/include/efi/x86_64

# Output files
KERNEL_ELF := kernel.elf
UEFI_IMAGE := ecomos-uefi.img

# =============================================================================
# BUILD TARGETS
# =============================================================================

.PHONY: all kernel uefi run clean

# Default target - build UEFI kernel
all: $(UEFI_IMAGE)
	@echo "✅ Build complete: $(UEFI_IMAGE)"

# Build kernel only
kernel: $(KERNEL_ELF)
	@echo "✅ Kernel ready: $(KERNEL_ELF)"

# Create UEFI bootable image
uefi: $(UEFI_IMAGE)
	@echo "✅ UEFI image: $(UEFI_IMAGE)"

# Test in QEMU
run: $(UEFI_IMAGE)
	@echo "🚀 Starting QEMU with UEFI..."
	$(QEMU) -drive file=$(UEFI_IMAGE),format=raw -m 512M -serial stdio

# Clean build artifacts
clean:
	@echo "🧹 Cleaning build files..."
	rm -f *.o *.elf *.img
	@echo "✅ Clean complete"

# =============================================================================
# BUILD RULES
# =============================================================================

# Link kernel
$(KERNEL_ELF): kernel_main.o src/kernel/main.o $(wildcard src/*/*.o) $(wildcard arch/x86_64/*/*.o)
	@echo "🔗 Linking kernel..."
	$(LD) $(LDFLAGS) -o $@ $^
	@echo "✅ Kernel size: $$(stat -f%z $@ 2>/dev/null || stat -c%s $@) bytes"

# Compile main entry point
kernel_main.o: kernel_main.c
	@echo "🔧 Compiling kernel entry point..."
	$(CC) $(CFLAGS) -Iinclude -Iarch/x86_64/internal $< -o $@

# Ensure src/kernel/main.c is compiled
src/kernel/main.o: src/kernel/main.c
	@echo "🔧 Compiling $<..."
	$(CC) $(CFLAGS) -Iinclude -Iarch/x86_64/internal $< -o $@

# Compile source files
%.o: %.c
	@echo "🔧 Compiling $<..."
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -Iinclude -Iarch/x86_64/internal $< -o $@

# Create UEFI image
$(UEFI_IMAGE): $(KERNEL_ELF)
	@echo "🔨 Creating UEFI bootable image..."
	# Create FAT32 image
	dd if=/dev/zero of=$@ bs=1M count=64 2>/dev/null
	mkfs.fat -F 32 $@ 2>/dev/null
	# Copy kernel as EFI application
	mmd -i $@ ::/EFI ::/EFI/BOOT 2>/dev/null || true
	mcopy -i $@ $(KERNEL_ELF) ::/EFI/BOOT/BOOTX64.EFI
	@echo "✅ UEFI image created: $@"