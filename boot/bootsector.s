; E-comOS Simple Boot Sector
; Minimal bootloader for canuse.img

[BITS 16]
[ORG 0x7C00]

_start:
    ; Set up segments
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    
    ; Load kernel from sector 2
    mov ah, 0x02            ; Read sectors
    mov al, 0x10            ; Read 16 sectors (8KB)
    mov ch, 0x00            ; Cylinder 0
    mov cl, 0x02            ; Start from sector 2
    mov dh, 0x00            ; Head 0
    mov dl, 0x00            ; Drive 0 (floppy)
    mov bx, 0x1000          ; Load to 0x1000
    int 0x13                ; BIOS disk interrupt
    
    ; Jump to kernel
    jmp 0x1000:0x0000
    
    ; Infinite loop if return
halt:
    hlt
    jmp halt

; Pad to 510 bytes and add boot signature
times 510-($-$$) db 0
dw 0xAA55