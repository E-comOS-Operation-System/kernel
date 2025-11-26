#!/bin/bash

# E-comOS Distribution Builder
# Clones userspace components and builds complete distro

set -e

DISTRO_DIR="distro-base"
KERNEL_BIN="build/kernel.bin"

echo "🚀 Building E-comOS Distribution..."

# Clean previous build
rm -rf "$DISTRO_DIR"
mkdir -p "$DISTRO_DIR"/{boot,userspace,config}

# Build kernel first
echo "📦 Building kernel..."
make kernel

# Copy kernel
cp "$KERNEL_BIN" "$DISTRO_DIR/boot/"

# Clone userspace components using config
echo "📥 Cloning userspace components..."
chmod +x scripts/clone-components.sh
./scripts/clone-components.sh

# Copy userspace components
echo "📋 Assembling userspace..."
cp -r temp-repos/* "$DISTRO_DIR/userspace/"

# Create bootloader config
echo "⚙️  Creating boot configuration..."
mkdir -p "$DISTRO_DIR/boot/grub"
cat > "$DISTRO_DIR/boot/grub/grub.cfg" << 'EOF'
menuentry "E-comOS" {
    multiboot /boot/kernel.bin
    boot
}
EOF

# Create distro info
cat > "$DISTRO_DIR/distro-info.txt" << 'EOF'
E-comOS Distribution Base
========================

This folder contains:
- /boot/          Kernel and bootloader
- /userspace/     User-space services
- /config/        System configuration

To customize:
1. Add your apps to /userspace/
2. Modify boot config in /boot/grub/
3. Package with: grub-mkrescue -o myos.iso distro-base/

Components:
- Kernel: E-comOS microkernel
- VGA Service: Text display driver
- Shell: Basic command interpreter  
- IPC Helper: Inter-process communication
EOF

echo "✅ Distribution built in $DISTRO_DIR/"
echo "💡 Add your components to $DISTRO_DIR/userspace/"
echo "🔥 Package with: grub-mkrescue -o myos.iso $DISTRO_DIR/"