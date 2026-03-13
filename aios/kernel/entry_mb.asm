[BITS 32]

MULTIBOOT_MAGIC  equ 0x1BADB002
MULTIBOOT_FLAGS  equ 0x00
MULTIBOOT_CHKSUM equ -(MULTIBOOT_MAGIC + MULTIBOOT_FLAGS)

section .multiboot
    dd MULTIBOOT_MAGIC
    dd MULTIBOOT_FLAGS
    dd MULTIBOOT_CHKSUM

section .text
global _start
global setup_idt
global idt_install_syscall
extern kernel_main
extern syscall_isr

IDT_BASE equ 0x20000

_start:
    mov esp, 0x9FFFF
    call kernel_main
.hang:
    jmp .hang

setup_idt:
    mov edi, IDT_BASE
    mov ecx, 512
    xor eax, eax
.zero:
    mov [edi], eax
    add edi, 4
    loop .zero
    mov ecx, 0
.fill:
    mov edi, IDT_BASE
    mov eax, ecx
    imul eax, 8
    add edi, eax
    mov eax, isr_stub
    mov word [edi], ax
    mov word [edi+2], 0x08
    mov byte [edi+4], 0
    mov byte [edi+5], 10001110b
    shr eax, 16
    mov word [edi+6], ax
    inc ecx
    cmp ecx, 256
    jl .fill
    mov word [idt_limit], 256 * 8 - 1
    mov dword [idt_base_addr], IDT_BASE
    lidt [idt_limit]
    ret

idt_install_syscall:
    mov edi, IDT_BASE + (0x80 * 8)
    mov eax, syscall_isr
    mov word [edi], ax
    mov word [edi+2], 0x08
    mov byte [edi+4], 0
    mov byte [edi+5], 10001110b
    shr eax, 16
    mov word [edi+6], ax
    ret

isr_stub:
    iret

idt_limit     dw 0
idt_base_addr dd 0
