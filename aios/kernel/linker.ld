ENTRY(_start)

SECTIONS
{
    . = 0x10000;

    .text : {
        kernel/entry.o(.text)
        *(.text)
    }

    .data : {
        kernel/entry.o(.data)
        *(.data)
    }

    .rodata : {
        *(.rodata)
    }

    .bss : {
        *(.bss)
    }
}
