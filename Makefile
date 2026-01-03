#  	E-comOS Kernel - A Microkernel for E-comOS
#   Copyright (C) 2025  Saladin5101
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

# E-comOS Makefile
# Core targets:
# - make kernel     → Build kernel binary
# - make image      → Generate base image canuse.img (kernel + minimal bootloader)
# - make run        → Test canuse.img with QEMU
# - make clean      → Clean all build artifacts
# - make fuckimage  → To create a folder to make your relase
# Try cross-compiler first, fallback to system compiler
CC = $(shell which x86_64-elf-gcc 2>/dev/null || echo "clang")
AS = $(shell which x86_64-elf-as 2>/dev/null || echo "nasm")
LD = $(shell which x86_64-elf-ld 2>/dev/null || echo "ld")
NASM = $(shell which nasm 2>/dev/null || echo "nasm")

# Compiler flags for 64-bit microkernel
ifeq ($(CC),clang)
    CFLAGS = -m64 -ffreestanding -fno-builtin -fno-stack-protector \
             -nostdlib -Wall -Wextra -c -mcmodel=large
    LINK_CMD = $(CC) -m64 -nostdlib -ffreestanding
else
    CFLAGS = -m64 -ffreestanding -fno-builtin -fno-stack-protector \
             -nostdlib -Wall -Wextra -c -mcmodel=large
    LINK_CMD = $(CC) -m64 -nostdlib -ffreestanding
endif

LDFLAGS = -T arch/x86_64/boot/linker.ld
ASFLAGS = --64
BOOT_ASFLAGS = --32
NASM_BOOT_FLAGS = -f bin

# Source files
KERNEL_SOURCES = src/kernel/main.c src/ipc/ipc.c src/mm/mm.c src/sched/sched.c src/kernel/syscall.c
ASM_SOURCES = kernel_entry.s
STAGE1_SRC = /Users/ddd/DOS25/src/boot/bootsect.s
STAGE2_SRC = /Users/ddd/DOS25/src/boot/bootsect-2nd.s

# Object files
BOOT_OBJ = boot/boot.o
STAGE1_BIN = dos25-release.bin
STAGE2_BIN = dos25-stage2.bin
KERNEL_OBJS = $(KERNEL_SOURCES:.c=.o)
ASM_OBJS = $(ASM_SOURCES:.s=.o)
ALL_OBJS = $(ASM_OBJS) $(KERNEL_OBJS)

# Output files
IMAGE_FILE = canuse.img
ISO_FILE = ecomos.iso

# Directories
SRC_DIR = src
BUILD_DIR = build
ISO_DIR = iso
BOOT_DIR = $(ISO_DIR)/boot

# Files
KERNEL_ELF = $(BUILD_DIR)/kernel.elf
KERNEL_BIN = $(BUILD_DIR)/kernel.bin
ISO_FILE = canuse.img

# Default target
all: $(IMAGE_FILE)

# README Promise 1: make image (basic kernel + minimal bootloader)
image: $(IMAGE_FILE)
	@echo "✅ Basic E-comOS image ready: $(IMAGE_FILE)"
	@echo "   Size: $$(stat -f%z $(IMAGE_FILE) 2>/dev/null || stat -c%s $(IMAGE_FILE)) bytes"
	@echo "   Usage: qemu-system-i386 -fda $(IMAGE_FILE)"
	@echo "   Or: qemu-system-x86_64 -fda $(IMAGE_FILE)"

kernel: $(KERNEL_BIN)
	@echo "✅ E-comOS kernel binary ready: $(KERNEL_BIN)"
	@echo "   Size: $$(stat -f%z $(KERNEL_BIN) 2>/dev/null || stat -c%s $(KERNEL_BIN)) bytes"

# Generate kernel info for DOS25
kernel-info: $(KERNEL_BIN)
	@echo "# E-comOS Kernel Info for DOS25" > kernel-info.txt
	@echo "KERNEL_SIZE=$$(stat -f%z $(KERNEL_BIN) 2>/dev/null || stat -c%s $(KERNEL_BIN))" >> kernel-info.txt
	@echo "KERNEL_ENTRY=_start" >> kernel-info.txt
	@echo "LOAD_ADDRESS=0x100000" >> kernel-info.txt
	@echo "📝 Kernel info generated: kernel-info.txt"

$(KERNEL_BIN): $(ALL_OBJS)
	@echo "🔗 Linking microkernel..."
	@echo "   Compiler: $(CC)"
ifeq ($(CC),clang)
	@echo "⚠️  Creating object archive for macOS (testing only)"
	ar rcs $(KERNEL_BIN) $(filter-out boot/boot.o,$(ALL_OBJS))
	@echo "📝 Note: This creates an archive, not a bootable kernel"
	@echo "    Use a cross-compiler for actual bootable kernel"
else
	$(LINK_CMD) -T arch/x86_64/boot/linker.ld -o $(KERNEL_BIN) $(ALL_OBJS)
endif
	@echo "📏 Kernel size: $$(stat -f%z $(KERNEL_BIN) 2>/dev/null || stat -c%s $(KERNEL_BIN)) bytes"

# Assembly files
$(BOOT_OBJ): $(BOOT_ASM)
	@echo "🔧 Assembling boot code..."
	$(AS) $(BOOT_ASFLAGS) $< -o $@

# C source files
%.o: %.c
	@echo "🔧 Compiling $<..."
	$(CC) $(CFLAGS) -I include $< -o $@

# Assembly source files
%.o: %.s
	@echo "🔧 Assembling $<..."
	$(AS) $(ASFLAGS) $< -o $@

# Build boot sectors
$(STAGE1_BIN): $(STAGE1_SRC)
	@echo "🔧 Building stage1 (MBR)..."
	$(NASM) $(NASM_BOOT_FLAGS) $< -o $@
	@size=$$(wc -c < $@); \
	echo "  Stage1 size: $$size bytes"; \
	if [ $$size -ne 512 ]; then \
		echo "  ⚠️  Warning: Stage1 should be 512 bytes"; \
	fi

$(STAGE2_BIN): $(STAGE2_SRC)
	@echo "🔧 Building stage2 (13 sectors)..."
	$(NASM) $(NASM_BOOT_FLAGS) $< -o $@
	@size=$$(wc -c < $@); \
	echo "  Stage2 size: $$size bytes"; \
	if [ $$size -ne 6656 ]; then \
		echo "  ⚠️  Adjusting stage2 to 6656 bytes..."; \
		dd if=/dev/zero of=temp-stage2.bin bs=1 count=6656 2>/dev/null; \
		dd if=$@ of=temp-stage2.bin conv=notrunc 2>/dev/null; \
		mv temp-stage2.bin $@; \
		echo "  Adjusted stage2 size: $$(wc -c < $@) bytes"; \
	fi

# README Promise 2: make fuckimage (full distro base)
fuckimage: kernel
	@echo "🚀 Creating E-comOS distro base..."
	chmod +x scripts/build-distro.sh
	./scripts/build-distro.sh
	@echo "✅ Distribution ready in distro-base/"

# Development targets
clean:
	@echo "🧹 Cleaning build files..."
	rm -f $(ALL_OBJS) $(KERNEL_BIN) $(IMAGE_FILE) $(STAGE1_BIN) $(STAGE2_BIN) kernel-info.txt temp-stage2.bin
	rm -rf distro-base temp-repos

debug: CFLAGS += -g -DDEBUG
debug: $(IMAGE_FILE)

# Create simple test kernel
simple_kernel.bin: simple_kernel.s
	$(AS) -f bin $< -o $@

# Create test kernel
test_kernel16.bin: test_kernel16.s
	$(AS) -f bin $< -o $@

# Create 16-bit kernel
kernel16.bin: kernel16.s
	$(AS) -f bin $< -o $@

# Create bootable image
$(IMAGE_FILE): $(KERNEL_BIN) $(STAGE1_BIN) $(STAGE2_BIN)
	@echo "🔨 Creating bootable floppy image..."
	@echo "  Using:"
	@echo "    Stage1: $(STAGE1_BIN)"
	@echo "    Stage2: $(STAGE2_BIN)"
	@echo "    Kernel: $(KERNEL_BIN)"
	
	# Create 1.44MB floppy image
	dd if=/dev/zero of=$(IMAGE_FILE) bs=512 count=2880 2>/dev/null
	
	# Install boot sector (Stage1)
	dd if=$(STAGE1_BIN) of=$(IMAGE_FILE) bs=512 count=1 conv=notrunc 2>/dev/null
	@echo "  ✓ Stage1 written to sector 0"
	
	# Install stage2 (13 sectors starting at sector 1)
	dd if=$(STAGE2_BIN) of=$(IMAGE_FILE) bs=512 seek=1 conv=notrunc 2>/dev/null
	@echo "  ✓ Stage2 written to sectors 1-13"
	
	# Copy kernel to image (starting at sector 14)
	dd if=$(KERNEL_BIN) of=$(IMAGE_FILE) bs=512 seek=14 count=10 conv=notrunc 2>/dev/null
	@echo "  ✓ Kernel written to sectors 14-23"
	
	@echo "✅ Bootable image created: $(IMAGE_FILE)"
	@echo "  Total size: $$(stat -f%z $(IMAGE_FILE) 2>/dev/null || stat -c%s $(IMAGE_FILE)) bytes"

# Test the image
run: $(IMAGE_FILE)
	@echo "🧪 Testing E-comOS in QEMU..."
	@echo "  Stage1: $$(xxd -l 512 $(STAGE1_BIN) | grep -o "55 aa" | wc -l) boot signature"
	@echo "  Stage2: $$(wc -c < $(STAGE2_BIN)) bytes"
	@echo "  Image:  $$(wc -c < $(IMAGE_FILE)) bytes"
	qemu-system-x86_64 -drive file=$(IMAGE_FILE),format=raw -d int,cpu -no-reboot

# Quick test of boot process
boottest: $(STAGE1_BIN) $(STAGE2_BIN)
	@echo "🧪 Testing boot process only..."
	dd if=/dev/zero of=boottest.img bs=512 count=2880 2>/dev/null
	dd if=$(STAGE1_BIN) of=boottest.img conv=notrunc 2>/dev/null
	dd if=$(STAGE2_BIN) of=boottest.img bs=512 seek=1 conv=notrunc 2>/dev/null
	@echo "  Boot test image created: boottest.img"
	qemu-system-x86_64 -drive file=boottest.img,format=raw

# Show kernel info
info:
	@echo "📊 E-comOS Microkernel Info:"
	@echo "   Target size: < 32KB"
	@echo "   System calls: 5"
	@echo "   Philosophy: Everything is service , everything is objects , everything is process"
	@if [ -f $(KERNEL_BIN) ]; then \
		echo "   Current size: $$(stat -f%z $(KERNEL_BIN) 2>/dev/null || stat -c%s $(KERNEL_BIN)) bytes"; \
	fi
	@if [ -f $(STAGE1_BIN) ]; then \
		echo "   Stage1: $$(wc -c < $(STAGE1_BIN)) bytes"; \
	fi
	@if [ -f $(STAGE2_BIN) ]; then \
		echo "   Stage2: $$(wc -c < $(STAGE2_BIN)) bytes"; \
	fi
	@if [ -f $(IMAGE_FILE) ]; then \
		echo "   Image: $$(wc -c < $(IMAGE_FILE)) bytes"; \
	fi

.PHONY: all image kernel fuckimage clean debug run kernel-info info boottest