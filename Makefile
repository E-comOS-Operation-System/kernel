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
# Cross-platform compatible for macOS, Linux, and Windows (WSL/Cygwin)
# Core targets:
# - make kernel     → Build kernel binary
# - make image      → Generate base image canuse.img (kernel + minimal bootloader)
# - make run        → Test canuse.img with QEMU
# - make clean      → Clean all build artifacts
# - make fuckimage  → To create a folder to make your release

# Detect OS
UNAME := $(shell uname -s)

# Toolchain - prefer cross-compiler for consistency
ifeq ($(UNAME),Darwin)
    # macOS
    CC = $(shell which x86_64-elf-gcc 2>/dev/null || which clang 2>/dev/null || echo "gcc")
    AS = $(shell which x86_64-elf-as 2>/dev/null || which as 2>/dev/null || echo "as")
    LD = $(shell which x86_64-elf-ld 2>/dev/null || which ld 2>/dev/null || echo "ld")
    OBJCOPY = $(shell which x86_64-elf-objcopy 2>/dev/null || which objcopy 2>/dev/null || echo "objcopy")
    QEMU = qemu-system-x86_64
    STAT_SIZE = stat -f%z
else ifeq ($(UNAME),Linux)
    # Linux
    CC = $(shell which x86_64-elf-gcc 2>/dev/null || which gcc 2>/dev/null)
    AS = $(shell which x86_64-elf-as 2>/dev/null || which as 2>/dev/null)
    LD = $(shell which x86_64-elf-ld 2>/dev/null || which ld 2>/dev/null)
    OBJCOPY = $(shell which x86_64-elf-objcopy 2>/dev/null || which objcopy 2>/dev/null)
    QEMU = qemu-system-x86_64
    STAT_SIZE = stat -c%s
else
    # Windows (Cygwin/MinGW/WSL)
    CC = gcc
    AS = as
    LD = ld
    OBJCOPY = objcopy
    QEMU = qemu-system-x86_64.exe
    STAT_SIZE = stat -c%s
endif

# NASM for assembly files
NASM = nasm

# Compiler flags
CFLAGS = -m64 -ffreestanding -fno-builtin -fno-stack-protector \
         -nostdlib -Wall -Wextra -c -mcmodel=large

# Assembler flags (for AS - GNU as)
ASFLAGS = --64

# NASM flags (for NASM assembler)
NASM_ELF64_FLAGS = -f elf64           # For kernel ELF objects
NASM_BIN_FLAGS = -f bin               # For boot sector (raw binary)

# Linker flags
LDFLAGS = -m elf_x86_64 -nostdlib -T arch/x86_64/boot/linker.ld

# Source files
KERNEL_SOURCES = src/kernel/main.c \
                 src/ipc/ipc.c \
                 src/mm/mm.c \
                 src/sched/sched.c \
                 src/kernel/syscall.c

# Assembly source files

ASM_SOURCES = kernel_entry.s

# Boot sector source files
STAGE1_SRC = src/boot/bootsect.s
STAGE2_SRC = src/boot/bootsect-2nd.s

# Object files
KERNEL_OBJS = $(KERNEL_SOURCES:.c=.o)

ALL_OBJS = $(KERNEL_OBJS) $(ASM_OBJS)

# Boot binaries
STAGE1_BIN = dos25-release.bin
STAGE2_BIN = dos25-stage2.bin
KERNEL_BIN = kernel.bin
IMAGE_FILE = canuse.img

# Default target
all: $(IMAGE_FILE)

# Kernel target
kernel: $(KERNEL_BIN)
	@echo "✅ E-comOS kernel binary ready: $(KERNEL_BIN)"
	@echo "   Size: $$($(STAT_SIZE) $(KERNEL_BIN) 2>/dev/null || wc -c < $(KERNEL_BIN)) bytes"

# Create bootable image
image: $(IMAGE_FILE)
	@echo "✅ E-comOS bootable image ready: $(IMAGE_FILE)"
	@echo "   Use: $(QEMU) -fda $(IMAGE_FILE)"

# Create stage1 boot sector
$(STAGE1_BIN): $(STAGE1_SRC)
	@echo "🔧 Building Stage 1 (MBR)..."
	$(NASM) $(NASM_BIN_FLAGS) $< -o $@
	@size=$$(wc -c < $@ 2>/dev/null || echo 0); \
	if [ $$size -ne 512 ]; then \
		echo "⚠️  Warning: Stage1 size is $$size bytes (should be 512)"; \
		if [ $$size -gt 512 ]; then \
			echo "❌ Error: Stage1 too large! Truncating..."; \
			dd if=$@ of=$@.tmp bs=1 count=512 2>/dev/null; \
			mv $@.tmp $@; \
		else \
			echo "   Padding to 512 bytes..."; \
			dd if=/dev/zero bs=1 count=$$((512 - $$size)) >> $@ 2>/dev/null; \
		fi; \
		echo "   Boot signature at offset 510..."; \
		printf "\x55\xAA" | dd of=$@ bs=1 seek=510 conv=notrunc 2>/dev/null; \
	fi
	@echo "   Stage1: $$($(STAT_SIZE) $@ 2>/dev/null || wc -c < $@) bytes"

# Create stage2 boot sector
$(STAGE2_BIN): $(STAGE2_SRC)
	@echo "🔧 Building Stage 2 (boot loader)..."
	$(NASM) $(NASM_BIN_FLAGS) $< -o $@
	@echo "   Stage2: $$($(STAT_SIZE) $@ 2>/dev/null || wc -c < $@) bytes"
	@# Ensure stage2 is multiple of 512 bytes
	@size=$$(wc -c < $@ 2>/dev/null || echo 0); \
	padding=$$(( (512 - ($$size % 512)) % 512 )); \
	if [ $$padding -ne 0 ]; then \
		echo "   Padding $$padding bytes..."; \
		dd if=/dev/zero bs=1 count=$$padding >> $@ 2>/dev/null; \
	fi

# Compile C source files
%.o: %.c
	@echo "🔧 Compiling $<..."
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -I include -o $@ $<

# Assemble assembly source files (GAS/AT&T syntax)
%.o: %.s
	@echo "🔧 Assembling $< with GNU as..."
	as $(ASFLAGS) -o $@ $<


# Link kernel
$(KERNEL_BIN): $(ALL_OBJS)
	@echo "🔗 Linking kernel..."
	@if [ -z "$(shell which $(LD) 2>/dev/null)" ]; then \
		echo "❌ Error: Linker not found!"; \
		echo "   Please install binutils or cross-compiler toolchain."; \
		echo "   macOS: brew install x86_64-elf-binutils"; \
		echo "   Linux: sudo apt-get install binutils"; \
		exit 1; \
	fi
	$(LD) $(LDFLAGS) -o $@ $(ALL_OBJS)
	@echo "   Kernel size: $$($(STAT_SIZE) $@ 2>/dev/null || wc -c < $@) bytes"

# Create bootable image
$(IMAGE_FILE): $(STAGE1_BIN) $(STAGE2_BIN) $(KERNEL_BIN)
	@echo "🔨 Creating bootable floppy image..."
	@echo "  Using:"
	@echo "    Stage1: $(STAGE1_BIN)"
	@echo "    Stage2: $(STAGE2_BIN)"
	@echo "    Kernel: $(KERNEL_BIN)"
	
	# Create 1.44MB floppy image
	@dd if=/dev/zero of=$(IMAGE_FILE) bs=512 count=2880 2>/dev/null
	
	# Install boot sector (Stage1)
	@dd if=$(STAGE1_BIN) of=$(IMAGE_FILE) bs=512 count=1 conv=notrunc 2>/dev/null
	@echo "  ✓ Stage1 written to sector 0"
	
	# Install stage2 (starting at sector 1)
	@dd if=$(STAGE2_BIN) of=$(IMAGE_FILE) bs=512 seek=1 conv=notrunc 2>/dev/null
	@echo "  ✓ Stage2 written to sectors 1-$$((1 + $$(($$($(STAT_SIZE) $(STAGE2_BIN) 2>/dev/null || wc -c < $(STAGE2_BIN)) / 512))))"
	
	# Copy kernel to image (starting at sector 14)
	@dd if=$(KERNEL_BIN) of=$(IMAGE_FILE) bs=512 seek=14 conv=notrunc 2>/dev/null
	@echo "  ✓ Kernel written starting at sector 14"
	
	@echo "✅ Bootable image created: $(IMAGE_FILE)"
	@echo "  Total size: $$($(STAT_SIZE) $(IMAGE_FILE) 2>/dev/null || wc -c < $(IMAGE_FILE)) bytes"

# Test in QEMU
run: $(IMAGE_FILE)
	@echo "🚀 Testing E-comOS in QEMU..."
	@echo "  Image: $(IMAGE_FILE)"
	@echo "  QEMU: $(QEMU)"
	$(QEMU) -fda $(IMAGE_FILE) -monitor stdio

# Test with debug output
debug: $(IMAGE_FILE)
	@echo "🐛 Debugging E-comOS in QEMU..."
	$(QEMU) -fda $(IMAGE_FILE) -d int,cpu_reset -D qemu.log

# Quick boot test
boottest: $(STAGE1_BIN) $(STAGE2_BIN)
	@echo "🧪 Testing boot process only..."
	@dd if=/dev/zero of=boottest.img bs=512 count=2880 2>/dev/null
	@dd if=$(STAGE1_BIN) of=boottest.img conv=notrunc 2>/dev/null
	@dd if=$(STAGE2_BIN) of=boottest.img bs=512 seek=1 conv=notrunc 2>/dev/null
	@echo "  Boot test image created: boottest.img"
	$(QEMU) -fda boottest.img

# Generate kernel info
kernel-info: $(KERNEL_BIN)
	@echo "# E-comOS Kernel Info" > kernel-info.txt
	@echo "KERNEL_SIZE=$$($(STAT_SIZE) $(KERNEL_BIN) 2>/dev/null || wc -c < $(KERNEL_BIN))" >> kernel-info.txt
	@echo "KERNEL_ENTRY=_start" >> kernel-info.txt
	@echo "LOAD_ADDRESS=0x100000" >> kernel-info.txt
	@echo "📝 Kernel info generated: kernel-info.txt"

# Clean build artifacts
clean:
	@echo "🧹 Cleaning build files..."
	@rm -f $(KERNEL_OBJS) $(ASM_OBJS) $(KERNEL_BIN) $(STAGE1_BIN) $(STAGE2_BIN) $(IMAGE_FILE) boottest.img
	@rm -f *.o *.nasmo */*.o */*/*.o kernel-info.txt qemu.log
	@echo "✅ Clean complete"

# Distribution target
fuckimage: all
	@echo "📦 Creating fuckimage distribution..."
	@mkdir -p fuckimage
	@cp $(IMAGE_FILE) fuckimage/ecomos.img
	@cp README.md fuckimage/ 2>/dev/null || true
	@echo "E-comOS Image" > fuckimage/INFO.txt
	@echo "=============" >> fuckimage/INFO.txt
	@echo "Created: $(shell date)" >> fuckimage/INFO.txt
	@echo "Size: $$($(STAT_SIZE) $(IMAGE_FILE) 2>/dev/null || wc -c < $(IMAGE_FILE)) bytes" >> fuckimage/INFO.txt
	@echo "✅ Distribution ready in fuckimage/ directory"

# Help target
help:
	@echo "E-comOS Makefile Help"
	@echo ""
	@echo "Available targets:"
	@echo "  make all        - Build everything (default)"
	@echo "  make kernel     - Build kernel only"
	@echo "  make image      - Create bootable image"
	@echo "  make run        - Test in QEMU"
	@echo "  make debug      - Test with debug output"
	@echo "  make boottest   - Test boot process only"
	@echo "  make kernel-info - Generate kernel info"
	@echo "  make clean      - Clean build artifacts"
	@echo "  make fuckimage   - Create distribution package"
	@echo "  make help       - Show this help"
	@echo ""
	@echo "Detected OS: $(UNAME)"
	@echo "Compiler: $(CC)"
	@echo "Assembler: $(AS) (for GAS/AT&T syntax)"
	@echo "NASM: $(NASM) (for NASM syntax boot sectors)"
	@echo "Linker: $(LD)"
	@echo "QEMU: $(QEMU)"
	@echo ""
	@echo "Assembly file types:"
	@echo "  *.s files: Compiled with GNU as (GAS/AT&T syntax)"
	@echo "  bootsect*.s files: Compiled with NASM (Intel syntax)"

.PHONY: all kernel image run debug boottest kernel-info clean fuckimage help