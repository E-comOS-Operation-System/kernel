#  	E-comOS - A Microkernel-based Operating System
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
# - make image      → Generate base image canuse.img (kernel + minimal bootloader)
# - make fuckimage  → Generate full distro folder distro-base/ (with user-space services, config templates)
# - make run        → Test canuse.img with QEMU
# - make clean      → Clean all build artifacts


# e-comos-kernel/Makefile (updated)
ARCH := x86_64
CC := clang
LD := ld.lld
BUILD_DIR := build
KERNEL_BIN := $(BUILD_DIR)/e-comos-kernel

# Compilation flags: 64-bit, no stdlib, include kernel/ directory
CFLAGS := -std=c11 -ffreestanding -nostdlib -Wall -Werror -m64 -Ikernel
# Link flags: kernel entry at 0x100000 (matches DOS25 load address)
LDFLAGS := -Ttext 0x100000 -nostdlib

# Kernel source files (add mm.c and sched.c)
KERNEL_SRCS := kernel/kernel.c \
               kernel/mm.c \
               kernel/sched.c
# Generate object files (build/%.o from kernel/%.c)
KERNEL_OBJS := $(patsubst kernel/%.c, $(BUILD_DIR)/%.o, $(KERNEL_SRCS))

# Compile each .c file to .o
$(KERNEL_OBJS): $(BUILD_DIR)/%.o: kernel/%.c
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Link all objects to kernel binary
$(KERNEL_BIN): $(KERNEL_OBJS)
	$(LD) $(LDFLAGS) $^ -o $@
	@echo "✅ Kernel built: $(KERNEL_BIN) (loaded at 0x100000)"

# Default target: build kernel
all: $(KERNEL_BIN)

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR)

# Phony targets (avoid file name conflicts)
.PHONY: all clean