# =============================================================================
# E-comOS Kernel - A Microkernel for E-comOS
# Copyright (C) 2025,2026 Saladin5101
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published
# by the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
# =============================================================================

# Core targets:
# - make all      → Build everything (kernel + UEFI image)
# - make kernel   → Build kernel binary only  
# - make uefi     → Create UEFI bootable image
# - make run      → Test in QEMU with OVMF
# - make clean    → Clean all build artifacts
# - make help     → Show help message

# =============================================================================
# CONFIGURATION
# =============================================================================

# Cross-compiler toolchain for x86_64-elf
PREFIX := x86_64-elf-
CC := $(PREFIX)gcc
LD := $(PREFIX)ld
OBJCOPY := $(PREFIX)objcopy
AS := $(PREFIX)gcc  # Use gcc for assembling .s files
QEMU := qemu-system-x86_64

# Check if cross-compiler is available
ifeq (, $(shell which $(CC) 2>/dev/null))
    $(error x86_64-elf-gcc not found! Please install cross-compiler toolchain)
endif

# UEFI-specific flags
CFLAGS := -m64 -ffreestanding -fno-builtin -fno-stack-protector \
          -nostdlib -Wall -Wextra -c -mcmodel=large -mno-red-zone \
          -fno-pic -fno-pie

# Assembly flags
ASFLAGS := -m64 -c -x assembler-with-cpp

# Linker flags
LDFLAGS := -m elf_x86_64 -nostdlib -T uefi.ld -z max-page-size=0x1000

# Add include directories to compiler flags
INCLUDE_DIRS := -Iinclude -Iinclude/kernel -Iinclude/kernel/arch \
                -Iarch/x86_64/internal
CFLAGS += $(INCLUDE_DIRS)
ASFLAGS += $(INCLUDE_DIRS)

# Output files
KERNEL_ELF := kernel.elf
UEFI_IMAGE := ecomos-uefi.img

# Source files
C_SOURCES := \
    kernel_main.c \
    src/kernel/main.c \
    src/kernel/syscall.c \
    src/mm/mm.c \
    src/ipc/ipc.c \
    src/sched/sched.c \
    src/printkit/print.c \
    src/time/time.c

ASM_SOURCES :=

# Object files
C_OBJECTS := $(C_SOURCES:.c=.o)
ASM_OBJECTS := $(ASM_SOURCES:.s=.o)
ALL_OBJECTS := $(C_OBJECTS) $(ASM_OBJECTS)

# =============================================================================
# PHONY TARGETS
# =============================================================================

.PHONY: all kernel uefi run clean help check-toolchain

# Default target - build UEFI kernel
all: check-toolchain $(UEFI_IMAGE)
	@echo "✅ Build complete: $(UEFI_IMAGE)"

# Check toolchain availability
check-toolchain:
	@echo "🔧 Checking for x86_64-elf toolchain..."
	@which $(CC) >/dev/null || (echo "❌ Error: $(CC) not found!"; exit 1)
	@echo "✅ Toolchain found: $(shell $(CC) --version | head -n1)"

# Build kernel only
kernel: check-toolchain $(KERNEL_ELF)
	@echo "✅ Kernel ready: $(KERNEL_ELF)"

# Create UEFI bootable image
uefi: check-toolchain $(UEFI_IMAGE)
	@echo "✅ UEFI image: $(UEFI_IMAGE)"

# Test in QEMU
run: $(UEFI_IMAGE)
	@echo "🚀 Starting QEMU with UEFI..."
	$(QEMU) -drive file=$(UEFI_IMAGE),format=raw -m 512M -serial stdio \
        -machine q35 -cpu qemu64 -smp 1

# Clean build artifacts
clean:
	@echo "🧹 Cleaning build files..."
	@rm -f $(ALL_OBJECTS) $(KERNEL_ELF) $(UEFI_IMAGE)
	@echo "✅ Clean complete"

# Show help
help:
	@echo "E-comOS Kernel Build System"
	@echo ""
	@echo "Available targets:"
	@echo "  make all    - Build kernel and UEFI image (default)"
	@echo "  make kernel - Build kernel binary only"
	@echo "  make uefi   - Create UEFI bootable image"
	@echo "  make run    - Test in QEMU with OVMF"
	@echo "  make clean  - Clean all build artifacts"
	@echo "  make help   - Show this help message"
	@echo ""
	@echo "Toolchain:"
	@echo "  CC: $(CC)"
	@echo "  LD: $(LD)"
	@echo "  OBJCOPY: $(OBJCOPY)"
	@echo "  AS: $(AS)"

# =============================================================================
# BUILD RULES
# =============================================================================

# Link kernel with all object files
$(KERNEL_ELF): $(ALL_OBJECTS)
	@echo "🔗 Linking kernel..."
	$(LD) $(LDFLAGS) -o $@ $(ALL_OBJECTS)
	@echo "✅ Kernel size: $$(stat -f%z $@ 2>/dev/null || stat -c%s $@) bytes"
	@echo "📦 Linked object files:"
	@for obj in $(ALL_OBJECTS); do \
		if [ -f "$$obj" ]; then \
			echo "  ✓ $$obj"; \
		fi; \
	done

# Compile C source files
%.o: %.c
	@echo "🔧 Compiling $<..."
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $< -o $@

# Compile assembly files (using gcc instead of as directly)
# This ensures proper preprocessing and flag handling
%.o: %.s
	@echo "🔧 Assembling $< (GAS)..."
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@

# Alternative: If you want to use NASM instead, comment the above and uncomment below
# NASM := nasm
# NASMFLAGS := -f elf64
# %.o: %.s
#	@echo "🔧 Assembling $< (NASM)..."
#	@mkdir -p $(dir $@)
#	$(NASM) $(NASMFLAGS) $< -o $@

# Create UEFI image
$(UEFI_IMAGE): $(KERNEL_ELF)
	@echo "🔨 Creating UEFI bootable image..."
	# Create FAT32 image
	dd if=/dev/zero of=$@ bs=1M count=64 status=none
	mkfs.fat -F 32 $@ >/dev/null 2>&1
	# Copy kernel as EFI application
	mmd -i $@ ::/EFI ::/EFI/BOOT 2>/dev/null || true
	mcopy -i $@ $(KERNEL_ELF) ::/EFI/BOOT/BOOTX64.EFI
	@echo "✅ UEFI image created: $@"