[BITS 32]


_start:
    mov esp, 0x9FFFF
    call serial_init
    mov esi, banner
    call serial_print
    call vga_clear
    mov esi, banner
    mov bl, 0x0A
    call vga_print
    call pic_init
    call idt_init
    call pit_init
    mov esi, msg_hw_ok
    call serial_print
    mov esi, msg_hw_ok
    mov bl, 0x0F
    call vga_print
    sti
.hang2:

.hang:
    jmp .hang

times 8192 - ($ - $$) db 0


; == DATA ==
jmp _start

banner    db "============================================", 0x0A
          db "        AIOS Kernel - Assembly Layer        ", 0x0A
          db "============================================", 0x0A, 0

msg_hw_ok db "VGA OK | PIC OK | PIT OK | IDT OK | IRQ OK", 0x0A
          db "Keyboard ready. Type below:", 0x0A
          db "--------------------------------------------", 0x0A, 0

exc_msg   db "CPU EXCEPTION!", 0x0A, 0

vga_row    dd 0
vga_col    dd 0
tick_count dd 0

idt_descriptor dw 0
               dd 0

scancode_table:
    db 0,  0,  '1','2','3','4','5','6','7','8','9','0','-','=', 0,  0
    db 'q','w','e','r','t','y','u','i','o','p','[',']', 0,  0,  'a','s'
    db 'd','f','g','h','j','k','l',';', 39, '`', 0, 92,'z','x','c','v'
    db 'b','n','m',',','.','/', 0,  '*', 0,  ' ', 0

SERIAL equ 0x3F8

serial_init:
    mov dx, SERIAL + 1
    mov al, 0
    out dx, al
    mov dx, SERIAL + 3
    mov al, 0x80
    out dx, al
    mov dx, SERIAL
    mov al, 1
    out dx, al
    mov dx, SERIAL + 1
    mov al, 0
    out dx, al
    mov dx, SERIAL + 3
    mov al, 3
    out dx, al
    mov dx, SERIAL + 2
    mov al, 0xC7
    out dx, al
    ret

serial_putchar:
    push edx
    push eax
.wait:
    mov dx, SERIAL + 5
    in al, dx
    and al, 0x20
    jz .wait
    mov dx, SERIAL
    pop eax
    out dx, al
    pop edx
    ret

serial_print:
    pusha
.loop:
    lodsb
    or al, al
    jz .done
    call serial_putchar
    jmp .loop
.done:
    popa
    ret

VGA_BASE equ 0xB8000
VGA_COLS equ 80
VGA_ROWS equ 25

vga_clear:
    mov edi, VGA_BASE
    mov ecx, VGA_COLS * VGA_ROWS
    mov ax, 0x0720
.loop:
    mov [edi], ax
    add edi, 2
    loop .loop
    mov dword [vga_row], 0
    mov dword [vga_col], 0
    ret

vga_print:
    pusha
.loop:
    lodsb
    or al, al
    jz .done
    cmp al, 0x0A
    je .newline
    cmp al, 0x0D
    je .loop
    call vga_putchar
    jmp .loop
.newline:
    inc dword [vga_row]
    mov dword [vga_col], 0
    jmp .loop
.done:
    popa
    ret

vga_putchar:
    push eax
    push ebx
    push edi
    mov edi, VGA_BASE
    mov eax, [vga_row]
    imul eax, VGA_COLS * 2
    add edi, eax
    mov eax, [vga_col]
    imul eax, 2
    add edi, eax
    mov ah, bl
    mov [edi], ax
    inc dword [vga_col]
    cmp dword [vga_col], VGA_COLS
    jl .done
    mov dword [vga_col], 0
    inc dword [vga_row]
.done:
    pop edi
    pop ebx
    pop eax
    ret

vga_print_hex:
    pusha
    mov ecx, 8
.loop:
    rol eax, 4
    mov edx, eax
    and edx, 0xF
    cmp edx, 9
    jle .digit
    add edx, 'A' - 10
    jmp .print
.digit:
    add edx, '0'
.print:
    push eax
    mov al, dl
    call vga_putchar
    pop eax
    loop .loop
    popa
    ret

PIC1_CMD  equ 0x20
PIC1_DATA equ 0x21
PIC2_CMD  equ 0xA0
PIC2_DATA equ 0xA1

pic_init:
    mov al, 0x11
    out PIC1_CMD, al
    out PIC2_CMD, al
    mov al, 0x20
    out PIC1_DATA, al
    mov al, 0x28
    out PIC2_DATA, al
    mov al, 0x04
    out PIC1_DATA, al
    mov al, 0x02
    out PIC2_DATA, al
    mov al, 0x01
    out PIC1_DATA, al
    out PIC2_DATA, al
    mov al, 11111100b
    out PIC1_DATA, al
    mov al, 11111111b
    out PIC2_DATA, al
    ret

PIT_CH0 equ 0x40
PIT_CMD equ 0x43
PIT_DIV equ 11932

pit_init:
    mov al, 00110110b
    out PIT_CMD, al
    mov ax, PIT_DIV
    out PIT_CH0, al
    mov al, ah
    out PIT_CH0, al
    ret

IDT_BASE equ 0x10000

idt_init:
    mov edi, IDT_BASE
    mov ecx, 512
    xor eax, eax
.clear:
    mov [edi], eax
    add edi, 4
    loop .clear
    mov ecx, 0
.exceptions:
    mov edi, IDT_BASE
    mov eax, ecx
    imul eax, 8
    add edi, eax
    mov eax, isr_generic
    call idt_set_gate
    inc ecx
    cmp ecx, 8
    jl .exceptions
    mov edi, IDT_BASE + (0x20 * 8)
    mov eax, irq0_timer
    call idt_set_gate
    mov edi, IDT_BASE + (0x21 * 8)
    mov eax, irq1_keyboard
    call idt_set_gate
    mov word [idt_descriptor], 256 * 8 - 1
    mov dword [idt_descriptor + 2], IDT_BASE
    lidt [idt_descriptor]
    ret

idt_set_gate:
    mov word [edi], ax
    mov word [edi+2], 0x08
    mov byte [edi+4], 0
    mov byte [edi+5], 10001110b
    shr eax, 16
    mov word [edi+6], ax
    ret

irq0_timer:
    pusha
    inc dword [tick_count]
    mov dword [vga_row], 0
    mov dword [vga_col], 70
    mov eax, [tick_count]
    mov bl, 0x0E
    call vga_print_hex
    mov al, 0x20
    out PIC1_CMD, al
    popa
    iret

irq1_keyboard:
    pusha
    in al, 0x60
    test al, 0x80
    jnz .done
    movzx eax, al
    cmp eax, 58
    jge .done
    mov bl, [scancode_table + eax]
    or bl, bl
    jz .done
    mov al, bl
    call serial_putchar
    mov al, bl
    mov bl, 0x0F
    call vga_putchar
.done:
    mov al, 0x20
    out PIC1_CMD, al
    popa
    iret

isr_generic:
    pusha
    mov esi, exc_msg
    call serial_print
    mov esi, exc_msg
    mov bl, 0x0C
    call vga_print
    popa
    iret

