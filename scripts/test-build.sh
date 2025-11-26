#!/bin/bash
# E-comOS Build Test Script

echo "🧪 Testing E-comOS build system..."

# Test 1: Build kernel
echo "📦 Testing 'make kernel'..."
if make kernel; then
    echo "✅ Kernel build: PASS"
else
    echo "❌ Kernel build: FAIL"
    exit 1
fi

# Test 2: Build image
echo "💿 Testing 'make image'..."
if make image; then
    echo "✅ Image build: PASS"
else
    echo "❌ Image build: FAIL"
    exit 1
fi

# Test 3: Check file sizes
if [ -f "ecomos-kernel.bin" ]; then
    KERNEL_SIZE=$(stat -f%z ecomos-kernel.bin 2>/dev/null || stat -c%s ecomos-kernel.bin)
    echo "📏 Kernel size: $KERNEL_SIZE bytes"
    
    if [ $KERNEL_SIZE -lt 32768 ]; then
        echo "✅ Kernel size check: PASS (< 32KB)"
    else
        echo "⚠️  Kernel size check: WARNING (>= 32KB)"
    fi
fi

if [ -f "canuse.img" ]; then
    IMAGE_SIZE=$(stat -f%z canuse.img 2>/dev/null || stat -c%s canuse.img)
    echo "💿 Image size: $IMAGE_SIZE bytes"
    
    if [ $IMAGE_SIZE -eq 1474560 ]; then
        echo "✅ Image size check: PASS (1.44MB floppy)"
    else
        echo "❌ Image size check: FAIL (not 1.44MB)"
    fi
fi

echo ""
echo "🎉 Build test completed!"
echo "📋 README promises status:"
echo "   ✅ make image - Creates canuse.img"
echo "   ⏳ make fuckimage - Reserved for user implementation"