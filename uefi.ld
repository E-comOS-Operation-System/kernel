ENTRY(_start)
OUTPUT_FORMAT(pe-x86-64)
OUTPUT_ARCH(i386:x86-64)

SECTIONS
{
    . = 0x100000;

    _kernelStart = .;

    .text : {
        *(.text)
    }

    .rdata : {
        *(.rdata)
    }

    .data : {
        *(.data)
    }

    .pdata : {
        *(.pdata)
    }

    .bss : {
        *(.bss)
        *(COMMON)
    }

    /DISCARD/ : {
        *(.eh_frame)
    }

    .reloc : {
        *(.reloc)
    }

    . = ALIGN(4096);
    _kernelEnd = .;
}