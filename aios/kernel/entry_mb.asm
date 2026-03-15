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
    mov esp, 0x1FFFF0
    ; Save multiboot info pointer (GRUB passes in ebx)
    mov [mbi_ptr], ebx
    lgdt [our_gdt_desc]
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    jmp 0x08:.flush_cs
.flush_cs:
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


global our_gdt
our_gdt:
    dq 0x0000000000000000   ; 0x00 null
    dq 0x00CF9A000000FFFF   ; 0x08 ring0 code
    dq 0x00CF92000000FFFF   ; 0x10 ring0 data
    dq 0x00CFFA000000FFFF   ; 0x18 ring3 code (DPL=3)
    dq 0x00CFF2000000FFFF   ; 0x20 ring3 data (DPL=3)
    dq 0x0000000000000000   ; 0x28 TSS (filled at runtime)
    dq 0x0000000000000000   ; 0x30 TSS high (for alignment)
our_gdt_desc:
    dw 55
    dd our_gdt

isr_stub:
    push eax
    mov al, 0x20
    out 0x20, al
    pop eax
    iret

idt_limit     dw 0
idt_base_addr dd 0
global idt_install_timer
global pit_init

idt_install_timer:
    ; Mask all IRQs first to prevent spurious interrupts during setup
    mov al, 0xFF
    out 0x21, al
    out 0xA1, al
    push eax
    mov al, 0x54
    out 0x3F8, al
    pop eax
    ; Install timer_irq_handler at IDT[0x20] (IRQ0 = PIC remapped to 0x20)
    mov edi, IDT_BASE + (0x20 * 8)
    mov eax, timer_irq_handler
    mov word [edi],   ax
    mov word [edi+2], 0x08
    mov byte [edi+4], 0
    mov byte [edi+5], 10001110b
    shr eax, 16
    mov word [edi+6], ax

    ; Remap PIC: IRQ0-7 -> INT 0x20-0x27
    mov al, 0x11
    out 0x20, al
    out 0xA0, al
    mov al, 0x20
    out 0x21, al
    mov al, 0x28
    out 0xA1, al
    mov al, 0x04
    out 0x21, al
    mov al, 0x02
    out 0xA1, al
    mov al, 0x01
    out 0x21, al
    out 0xA1, al

    ; Unmask IRQ0 only
    mov al, 0xFE
    out 0x21, al
    mov al, 0xFF
    out 0xA1, al

    ; Enable interrupts
    ret

pit_init:
    ; Set PIT channel 0, rate = 1193180 / 100 = 11931 (~100Hz)
    mov al, 0x36
    out 0x43, al
    mov ax, 11931
    out 0x40, al
    mov al, ah
    out 0x40, al
    ret

timer_irq_handler:
    push eax
    add dword [timer_ticks_bss], 1
    ; Send EOI to PIC
    mov al, 0x20
    out 0x20, al
    pop eax
    iret

section .bss
global timer_ticks_bss
timer_ticks_bss resd 1

section .data
global mbi_ptr
mbi_ptr dd 0
