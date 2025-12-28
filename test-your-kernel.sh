#!/bin/bash
# test-your-kernel.sh

echo "Testing your existing DOS25 kernel..."
echo "======================================"

# 检查必要文件
if [ ! -f "boot/bootsector.bin" ]; then
    echo "Error: boot/bootsector.bin not found!"
    echo "Using 64-bit bootloader instead..."
    nasm -f bin boot64.asm -o boot64.bin
    BOOTLOADER=boot64.bin
else
    BOOTLOADER=boot/bootsector.bin
    echo "Using your bootloader: boot/bootsector.bin"
fi

if [ -f "build/kernel.bin" ]; then
    KERNEL=build/kernel.bin
    echo "Using your kernel: build/kernel.bin"
elif [ -f "ecomos-kernel.bin" ]; then
    KERNEL=ecomos-kernel.bin
    echo "Using your kernel: ecomos-kernel.bin"
else
    echo "Error: No kernel found!"
    echo "Please compile your kernel first:"
    echo "  make  # or cmake --build build/"
    exit 1
fi

# 创建磁盘镜像
echo "Creating disk image..."
dd if=/dev/zero of=test-dos25.img bs=512 count=2880
dd if=$BOOTLOADER of=test-dos25.img conv=notrunc
dd if=$KERNEL of=test-dos25.img bs=512 seek=2 conv=notrunc

echo ""
echo "Testing 64-bit boot..."
qemu-system-x86_64 -fda test-dos25.img -no-reboot -no-shutdown