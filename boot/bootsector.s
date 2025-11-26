; E-comOS Simple Boot Sector
; Minimal bootloader for canuse.img

[BITS 16]
[ORG 0x7C00]

_start:
    ; Save boot drive
    mov [boot_drive], dl
    
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
    mov dl, [boot_drive]    ; Use boot drive
    mov bx, 0x1000          ; Load to 0x1000
    int 0x13                ; BIOS disk interrupt
    jc disk_error           ; Jump if error
    
    ; Print success
    mov si, success_msg
    call print_string
    
    ; Print entering protected mode
    mov si, pmode_msg
    call print_string
    
    ; Print before VGA test
    mov si, vga_msg
    call print_string
    
    ; Test VGA write - overwrite first line
    mov ax, 0xB800
    mov es, ax
    mov di, 20  ; Column 10 of first line
    mov ax, 0x0754  ; 'T' with white on black
    stosw
    mov ax, 0x0745  ; 'E' with white on black
    stosw
    
    ; Reset segments
    xor ax, ax
    mov ds, ax
    mov es, ax
    
    ; Print done message
    mov si, done_msg
    call print_string
    
    ; Jump to kernel at 0x10000
    jmp 0x1000:0x0000
    
disk_error:
    mov si, error_msg
    call print_string
    jmp halt
    
print_string:
    lodsb
    or al, al
    jz done
    mov ah, 0x0E
    int 0x10
    jmp print_string
done:
    ret
    
boot_drive db 0
success_msg db 'Boot OK!', 0
pmode_msg db ' PMode...', 0
vga_msg db ' VGA...', 0
done_msg db ' Done!', 0
error_msg db 'Disk Error!', 0

; GDT
gdt_start:
gdt_null:
    dd 0x0
    dd 0x0
gdt_code:
    dw 0xFFFF       ; limit low
    dw 0x0000       ; base low
    db 0x00         ; base middle
    db 0x9A         ; access (present, ring 0, code, executable, readable)
    db 0xCF         ; flags + limit high (4KB granularity, 32-bit)
    db 0x00         ; base high
gdt_data:
    dw 0xFFFF       ; limit low
    dw 0x0000       ; base low
    db 0x00         ; base middle
    db 0x92         ; access (present, ring 0, data, writable)
    db 0xCF         ; flags + limit high (4KB granularity, 32-bit)
    db 0x00         ; base high
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start
    
    ; Infinite loop if return
halt:
    hlt
    jmp halt

; Pad to 510 bytes and add boot signature
times 510-($-$$) db 0
dw 0xAA55