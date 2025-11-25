# 用户态VGA服务设计

## 概述
VGA显示驱动作为独立的用户态服务运行，通过IPC为其他程序提供显示功能。

## 架构
```
应用程序 → IPC → VGA服务 → 内核映射 → VGA硬件
```

## VGA服务功能
- 文本模式显示
- 字符输出和格式化
- 屏幕清除和滚动
- 颜色控制

## 系统调用接口
- `SYS_DISPLAY_MAP_FRAMEBUFFER`: 映射VGA内存到用户空间
- `SYS_DISPLAY_REGISTER_SERVICE`: 注册显示服务
- `SYS_DISPLAY_UNREGISTER_SERVICE`: 注销显示服务

## 用户态VGA服务示例代码

```c
// vga_service.c - 独立仓库中的用户态VGA驱动
#include <stdint.h>
#include <kernel/api/display.h>

#define VGA_MEMORY 0xB8000
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

static uint16_t* vga_buffer;
static size_t cursor_x = 0;
static size_t cursor_y = 0;

int vga_service_init(void) {
    // 通过系统调用映射VGA内存
    vga_buffer = (uint16_t*)syscall(SYS_DISPLAY_MAP_FRAMEBUFFER, VGA_MEMORY, 4000);
    if (!vga_buffer) return -1;
    
    // 注册显示服务
    struct display_service_info info = {
        .capabilities = DISPLAY_CAP_TEXT_MODE | DISPLAY_CAP_COLOR,
        .width = VGA_WIDTH,
        .height = VGA_HEIGHT,
        .bpp = 4
    };
    
    return syscall(SYS_DISPLAY_REGISTER_SERVICE, &info);
}

void vga_putchar(char c, uint8_t color) {
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
        return;
    }
    
    const size_t index = cursor_y * VGA_WIDTH + cursor_x;
    vga_buffer[index] = (uint16_t)c | ((uint16_t)color << 8);
    
    if (++cursor_x >= VGA_WIDTH) {
        cursor_x = 0;
        cursor_y++;
    }
    
    if (cursor_y >= VGA_HEIGHT) {
        // 滚动屏幕
        for (size_t i = 0; i < (VGA_HEIGHT - 1) * VGA_WIDTH; i++) {
            vga_buffer[i] = vga_buffer[i + VGA_WIDTH];
        }
        cursor_y = VGA_HEIGHT - 1;
    }
}

// IPC消息处理循环
void vga_service_main(void) {
    while (1) {
        // 接收IPC消息
        // 处理显示请求
        // 发送响应
    }
}
```

## 部署
1. 将VGA服务编译为独立的用户态程序
2. 内核启动后自动加载VGA服务
3. 其他程序通过IPC与VGA服务通信