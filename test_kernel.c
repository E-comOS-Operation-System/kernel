// Simple test kernel
void kernel_main(void) {
    // Write "KERNEL" directly to VGA buffer
    volatile unsigned short* vga = (volatile unsigned short*)0xB8000;
    vga[0] = 0x0F4B; // 'K'
    vga[1] = 0x0F45; // 'E' 
    vga[2] = 0x0F52; // 'R'
    vga[3] = 0x0F4E; // 'N'
    vga[4] = 0x0F45; // 'E'
    vga[5] = 0x0F4C; // 'L'
    
    // Infinite loop
    while(1) {
        __asm__ volatile ("hlt");
    }
}