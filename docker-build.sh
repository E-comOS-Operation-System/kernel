#!/bin/bash
# E-comOS Docker Build Script

echo "🐳 Building E-comOS in Docker container..."
docker build -t ecomos-dev .

echo "🔨 Building kernel image..."
docker run --rm -v $(pwd):/kernel ecomos-dev

echo "✅ Build complete! Run with:"
echo "   qemu-system-x86_64 -fda canuse.img"