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

# Try cross-compiler first, fallback to system compiler
CC = $(shell which i686-elf-gcc 2>/dev/null || echo "clang")
AS = $(shell which i686-elf-as 2>/dev/null || echo "nasm")
LD = $(shell which i686-elf-ld 2>/dev/null || echo "ld")

# Compiler flags for microkernel
ifeq ($(CC),clang)
    CFLAGS = -m32 -ffreestanding -fno-builtin -fno-stack-protector \
             -nostdlib -Wall -Wextra -c -target i386-pc-none-elf
    LINK_CMD = $(CC) -m32 -nostdlib -ffreestanding
else
    CFLAGS = -m32 -ffreestanding -fno-builtin -fno-stack-protector \
             -nostdlib -Wall -Wextra -c
    LINK_CMD = $(CC) -m32 -nostdlib -ffreestanding
endif

LDFLAGS = -T arch/x86_64/boot/linker.ld
ASFLAGS = -f elf32

# Source files
BOOT_ASM = boot/boot.s
BOOTSECTOR_ASM = boot/bootsector.s
KERNEL_SOURCES = src/kernel/main.c src/kernel/early_init.c src/kernel/panic.c src/kernel/syscall.c
IPC_SOURCES = src/ipc/ipc.c
SCHED_SOURCES = src/sched/sched.c
MM_SOURCES = src/mm/mm.c
ARCH_SOURCES = src/arch/x86_64/interrupts.c arch/x86_64/cpu/gdt.c arch/x86_64/mm/paging.c

# Object files
BOOT_OBJ = boot/boot.o
BOOTSECTOR_BIN = boot/bootsector.bin
KERNEL_OBJS = $(KERNEL_SOURCES:.c=.o)
IPC_OBJS = $(IPC_SOURCES:.c=.o)
SCHED_OBJS = $(SCHED_SOURCES:.c=.o)
MM_OBJS = $(MM_SOURCES:.c=.o)
ARCH_OBJS = $(ARCH_SOURCES:.c=.o)
ALL_OBJS = $(BOOT_OBJ) $(KERNEL_OBJS) $(IPC_OBJS) $(SCHED_OBJS) $(MM_OBJS) $(ARCH_OBJS)

# Output files
KERNEL_BIN = ecomos-kernel.bin
IMAGE_FILE = canuse.img

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
	# Create archive for testing - not bootable but validates code
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
	$(AS) $(ASFLAGS) $< -o $@

# C source files
%.o: %.c
	@echo "🔧 Compiling $<..."
	$(CC) $(CFLAGS) -I include $< -o $@

# README Promise 2: make fuckimage (full distro base)
fuckimage: kernel
	@echo "🚀 Creating E-comOS distro base..."
	chmod +x scripts/build-distro.sh
	./scripts/build-distro.sh
	@echo "✅ Distribution ready in distro-base/"

# Development targets
clean:
	@echo "🧹 Cleaning build files..."
	rm -f $(ALL_OBJS) $(KERNEL_BIN) $(IMAGE_FILE) $(BOOTSECTOR_BIN) kernel-info.txt
	rm -rf distro-base temp-repos

debug: CFLAGS += -g -DDEBUG
debug: $(IMAGE_FILE)

# Create bootable image
$(IMAGE_FILE): $(KERNEL_BIN) $(BOOTSECTOR_BIN)
	@echo "🔨 Creating bootable floppy image..."
	# Create 1.44MB floppy image
	dd if=/dev/zero of=$(IMAGE_FILE) bs=1024 count=1440 2>/dev/null
	# Install boot sector
	dd if=$(BOOTSECTOR_BIN) of=$(IMAGE_FILE) bs=512 count=1 conv=notrunc 2>/dev/null
	# Copy kernel to image (sector 2 onwards)
	dd if=$(KERNEL_BIN) of=$(IMAGE_FILE) bs=512 seek=1 conv=notrunc 2>/dev/null
	@echo "✅ Bootable image created: $(IMAGE_FILE)"

# Build boot sector
$(BOOTSECTOR_BIN): $(BOOTSECTOR_ASM)
	@echo "🔧 Building boot sector..."
	$(AS) -f bin $< -o $@

# Test the image
run: $(IMAGE_FILE)
	@echo "🧪 Testing E-comOS in QEMU..."
	qemu-system-i386 -fda $(IMAGE_FILE) -serial stdio

# Show kernel info
info:
	@echo "📊 E-comOS Microkernel Info:"
	@echo "   Target size: < 32KB"
	@echo "   System calls: 5"
	@echo "   Philosophy: 万物皆服务、万物皆对象、万物皆进程"
	@if [ -f $(KERNEL_BIN) ]; then \
		echo "   Current size: $$(stat -f%z $(KERNEL_BIN) 2>/dev/null || stat -c%s $(KERNEL_BIN)) bytes"; \
	fi

.PHONY: all image kernel fuckimage clean debug run kernel-info info