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
# - make kernel     → Build kernel binary
# - make image      → Generate base image canuse.img (kernel + minimal bootloader)
# - make run        → Test canuse.img with QEMU
# - make clean      → Clean all build artifacts

# 编译器和工具
CC = i686-elf-gcc
AS = i686-elf-as
LD = i686-elf-gcc

# 编译标志
CFLAGS = -std=gnu99 -ffreestanding -O2 -Wall -Wextra -Iinclude
LDFLAGS = -ffreestanding -O2 -nostdlib -lgcc

# 源文件
ASM_SOURCES = boot/boot.s
C_SOURCES = src/kernel/main.c src/kernel/early_init.c src/kernel/debug.c

# 目标文件
OBJ_DIR = build
ASM_OBJECTS = $(ASM_SOURCES:%.s=$(OBJ_DIR)/%.o)
C_OBJECTS = $(C_SOURCES:%.c=$(OBJ_DIR)/%.o)
OBJECTS = $(ASM_OBJECTS) $(C_OBJECTS)

# 默认目标
all: kernel

# 创建构建目录
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)/boot $(OBJ_DIR)/src/kernel

# 编译汇编文件
$(OBJ_DIR)/%.o: %.s | $(OBJ_DIR)
	$(AS) $< -o $@

# 编译C文件
$(OBJ_DIR)/%.o: %.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# 链接内核
kernel: $(OBJECTS)
	$(LD) -T arch/x86_64/boot/linker.ld -o $(OBJ_DIR)/kernel.bin $(OBJECTS) $(LDFLAGS)

# 创建ISO镜像
image: kernel
	mkdir -p isodir/boot/grub
	cp $(OBJ_DIR)/kernel.bin isodir/boot/kernel.bin
	echo 'menuentry "E-comOS" {' > isodir/boot/grub/grub.cfg
	echo '    multiboot /boot/kernel.bin' >> isodir/boot/grub/grub.cfg
	echo '}' >> isodir/boot/grub/grub.cfg
	grub-mkrescue -o canuse.img isodir

# 运行QEMU测试
run: image
	qemu-system-i386 -cdrom canuse.img

# 清理构建文件
clean:
	rm -rf $(OBJ_DIR) isodir canuse.img

.PHONY: all kernel image run clean
