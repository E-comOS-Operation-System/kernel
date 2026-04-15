/*
    E-comOS Kernel - Print Utility
    Copyright (C) 2025,2026  Saladin5101

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as published
    by the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <kernel/printkit/print.h>

#define VGA_MEMORY ((volatile uint16_t *)0xB8000)

static int cursorX = 0;
static int cursorY = 0;

void clearScreen(uint8_t color) {
    (void)color;
    for (int i = 0; i < 80 * 25; i++)
        VGA_MEMORY[i] = 0x0720;
    cursorX = cursorY = 0;
}

void printChar(char c, uint8_t color) {
    if (c == '\n') {
        cursorX = 0;
        cursorY++;
    } else if (c == '\r') {
        cursorX = 0;
    } else {
        VGA_MEMORY[cursorY * 80 + cursorX] = ((uint16_t)color << 8) | (uint8_t)c;
        cursorX++;
    }
    if (cursorX >= 80) {
        cursorX = 0;
        cursorY++;
    }
    if (cursorY >= 25) {
        for (int i = 0; i < 80 * 24; i++)
            VGA_MEMORY[i] = VGA_MEMORY[i + 80];
        for (int i = 80 * 24; i < 80 * 25; i++)
            VGA_MEMORY[i] = 0x0F20;
        cursorY = 24;
    }
}

void printStr(const char *str, uint8_t color) {
    for (int i = 0; str[i] != '\0'; i++)
        printChar(str[i], color);
}

void printNum(uint32_t num, uint8_t color) {
    char  buf[12];
    char *ptr = buf + 11;
    *ptr = '\0';
    if (num == 0) {
        printChar('0', color);
        return;
    }
    while (num > 0) {
        *--ptr = '0' + (num % 10);
        num /= 10;
    }
    printStr(ptr, color);
}

void printHex(uint32_t num, uint8_t color) {
    const char *digits = "0123456789ABCDEF";
    printStr("0x", color);
    for (int i = 7; i >= 0; i--)
        printChar(digits[(num >> (i * 4)) & 0xF], color);
}
