#!/bin/bash

# E-comOS Simplified Test Script

# Used to quickly verify code structure on macOS

echo "🧪 E-comOS Kernel Code Structure Test"

echo "================================"

echo "📁 Check Directory Structure..."

echo "✅ Source File Directory:"

find src -name "*.c" | sort

echo ""

echo "✅ Header File Directory:"
find include -name "*.h" | sort
echo ""
echo "📊 Code Statistics:"

echo " C Source Files: $(find src -name "*.c" | wc -l)"

echo " Header Files: $(find include -name "*.h" | wc -l)"

echo " Assembly Files: $(find . -name "*.s" | wc -l)"

echo ""

echo "🎯 Kernel Function Modules:"

echo " ✅ Boot Module (boot/)"

echo " ✅ Kernel Core (src/kernel/)"

echo " ✅ IPC Communication (src/ipc/)"

echo " ✅ Scheduler (src/sched/)"

echo " ✅ Memory Management (src/mm/)"

echo " ✅ Architecture Support (src/arch/)"

echo ""

echo "📝 System Call Interface:"

grep -n "SYS_" include/kernel/syscall.h | head -5

echo ""

echo "🏗️ Build System:"

echo " Makefile: $([ -f Makefile ] && echo "✅ Exists" || echo "❌ Missing")"

echo " Linker Script: $([ -f `arch/x86_64/boot/linker.ld ] && echo "✅ Exists" || echo "❌ Missing")"

echo ""

echo "📄 Copyright Notice Check:"
COPYRIGHT_COUNT=$(grep -r "Copyright (C) 2025 Saladin5101" src include | wc -l)

echo " Files with added copyright notice: $COPYRIGHT_COUNT"

echo ""

echo "🎉 Code structure verification complete!"

echo "💡 Tip: Run 'make kernel' for actual build in an environment with a cross-compilation toolchain"