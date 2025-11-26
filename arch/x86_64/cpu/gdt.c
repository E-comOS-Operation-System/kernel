/*
 * E-comOS - Global Descriptor Table
 */

#include <stdint.h>

struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_middle;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high;
} __attribute__((packed));

static struct gdt_entry gdt[3];

void gdt_init(void) {
    // Null descriptor
    gdt[0] = (struct gdt_entry){0, 0, 0, 0, 0, 0};
    
    // Code segment
    gdt[1] = (struct gdt_entry){0xFFFF, 0, 0, 0x9A, 0xCF, 0};
    
    // Data segment  
    gdt[2] = (struct gdt_entry){0xFFFF, 0, 0, 0x92, 0xCF, 0};
}