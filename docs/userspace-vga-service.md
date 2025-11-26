# Userspace VGA Service Design

## Overview
VGA display driver runs as an independent userspace service, providing display functionality to other programs through IPC.

## Architecture
```
Applications → IPC → VGA Service → Kernel Mapping → VGA Hardware
```

## VGA Service Features
- Text mode display
- Character output and formatting
- Screen clearing and scrolling
- Color control

## System Call Interface
- `SYS_DISPLAY_MAP_FRAMEBUFFER`: Map VGA memory to userspace
- `SYS_DISPLAY_REGISTER_SERVICE`: Register display service
- `SYS_DISPLAY_UNREGISTER_SERVICE`: Unregister display service

## Userspace VGA Service Example Code

```c
// vga_service.c - Userspace VGA driver in separate repository
#include <stdint.h>
#include <kernel/api/display.h>

#define VGA_MEMORY 0xB8000
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

static uint16_t* vga_buffer;
static size_t cursor_x = 0;
static size_t cursor_y = 0;

int vga_service_init(void) {
    // Map VGA memory through system call
    vga_buffer = (uint16_t*)syscall(SYS_DISPLAY_MAP_FRAMEBUFFER, VGA_MEMORY, 4000);
    if (!vga_buffer) return -1;
    
    // Register display service
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
        // Scroll screen
        for (size_t i = 0; i < (VGA_HEIGHT - 1) * VGA_WIDTH; i++) {
            vga_buffer[i] = vga_buffer[i + VGA_WIDTH];
        }
        cursor_y = VGA_HEIGHT - 1;
    }
}

// IPC message processing loop
void vga_service_main(void) {
    while (1) {
        // Receive IPC messages
        // Process display requests
        // Send responses
    }
}
```

## Deployment
1. Compile VGA service as independent userspace program
2. Automatically load VGA service after kernel boot
3. Other programs communicate with VGA service through IPC