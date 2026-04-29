#!/bin/bash
echo "开始替换 E-com_os 为 E-comOS..."

# 要替换的文件列表
files=(
    "src/ipc/ipc.c"
    "src/kernel/address_space.c"
    "src/kernel/capability.c"
    "src/kernel/debug.c"
    "src/kernel/list.c"
    "src/kernel/main.c"
    "src/kernel/process.c"
    "src/kernel/service.c"
    "src/kernel/syscall.c"
    "src/mm/mm.c"
    "src/printkit/print.c"
    "src/sched/sched.c"
    "src/time/time.c"
    "include/kernel/address_space.h"
    "include/kernel/boot.h"
    "include/kernel/capability.h"
    "include/kernel/debug.h"
    "include/kernel/early_init.h"
    "include/kernel/ipc.h"
    "include/kernel/mm.h"
    "include/kernel/object.h"
    "include/kernel/process.h"
    "include/kernel/sched.h"
    "include/kernel/service.h"
    "include/kernel/syscall.h"
)

# 对每个文件进行替换
for file in "${files[@]}"; do
    if [ -f "$file" ]; then
        echo "处理: $file"
        # 备份原文件
        cp "$file" "$file.bak"
        # 替换字符串
        sed -i 's/E-com_os/E-comOS/g' "$file"
    else
        echo "警告: 文件不存在 - $file"
    fi
done

echo "替换完成！已创建 .bak 备份文件。"
