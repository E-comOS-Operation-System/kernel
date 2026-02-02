/* 
 * E-comOS Kernel - Print Utility Implementation
 * Copyright (C) 2025,2026 Saladin5101
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <kernel/printkit/print.h>

/* VGA text mode buffer */
#define VGA_MEMORY ((volatile uint16_t*)0xB8000)

/* Current cursor position */
static int cursor_x = 0;
static int cursor_y = 0;

void clear_screen(uint8_t color) {
    uint16_t blank = 0x0720; // Space character with light gray on black
    for (int i = 0; i < 80 * 25; i++) {
        VGA_MEMORY[i] = blank;
    }
    cursor_x = cursor_y = 0;
}

void putchar(char c, uint8_t color) {
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    } else if (c == '\r') {
        cursor_x = 0;
    } else {
        int pos = cursor_y * 80 + cursor_x;
        VGA_MEMORY[pos] = (color << 8) | c;
        cursor_x++;
    }
    
    /* Handle line wrapping */
    if (cursor_x >= 80) {
        cursor_x = 0;
        cursor_y++;
    }
    
    /* Handle screen scrolling */
    if (cursor_y >= 25) {
        for (int i = 0; i < 80 * 24; i++) {
            VGA_MEMORY[i] = VGA_MEMORY[i + 80];
        }
        for (int i = 80 * 24; i < 80 * 25; i++) {
            VGA_MEMORY[i] = 0x0F20;
        }
        cursor_y = 24;
    }
}

void print(const char* str, uint8_t color) {
    for (int i = 0; str[i] != '\0'; i++) {
        putchar(str[i], color);
    }
}

void print_number(uint32_t num, uint8_t color) {
    char buffer[12];
    char* ptr = buffer + 11;
    *ptr = '\0';
    
    if (num == 0) {
        putchar('0', color);
        return;
    }
    
    while (num > 0) {
        *--ptr = '0' + (num % 10);
        num /= 10;
    }
    
    print(ptr, color);
}

void print_hex(uint32_t num, uint8_t color) {
    const char* hex_digits = "0123456789ABCDEF";
    
    print("0x", color);
    
    for (int i = 7; i >= 0; i--) {
        uint8_t nibble = (num >> (i * 4)) & 0xF;
        putchar(hex_digits[nibble], color);
    }
}