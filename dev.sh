#!/bin/bash
# E-comOS 完整开发环境

case "$1" in
    "build")
        echo "🔨 构建内核..."
        docker run --rm -v $(pwd):/kernel ecomos-dev make clean
        docker run --rm -v $(pwd):/kernel ecomos-dev make image
        ;;
    "test")
        echo "🧪 测试内核..."
        docker run --rm -v $(pwd):/kernel ecomos-dev qemu-system-x86_64 -fda canuse.img -nographic -serial mon:stdio
        ;;
    "debug")
        echo "🐛 调试模式..."
        docker run --rm -v $(pwd):/kernel -p 1234:1234 ecomos-dev qemu-system-x86_64 -fda canuse.img -s -S -nographic
        ;;
    "shell")
        echo "💻 进入开发环境..."
        docker run --rm -it -v $(pwd):/kernel ecomos-dev /bin/bash
        ;;
    "setup")
        echo "🐳 构建开发容器..."
        docker build -t ecomos-dev .
        ;;
    *)
        echo "E-comOS 开发工具"
        echo "用法: $0 {setup|build|test|debug|shell}"
        echo "  setup  - 构建Docker容器"
        echo "  build  - 编译内核"
        echo "  test   - 运行测试"
        echo "  debug  - 调试模式"
        echo "  shell  - 进入开发环境"
        ;;
esac