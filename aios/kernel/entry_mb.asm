[BITS 32]

MULTIBOOT_MAGIC   equ 0x1BADB002
MULTIBOOT_FLAGS   equ 0x00
MULTIBOOT_CHKSUM  equ -(MULTIBOOT_MAGIC + MULTIBOOT_FLAGS)

section .multiboot
    dd MULTIBOOT_MAGIC
    dd MULTIBOOT_FLAGS
    dd MULTIBOOT_CHKSUM

section .text
global _start
extern kernel_main

_start:
    mov esp, 0x9FFFF
    call kernel_main
.hang:
    jmp .hang
