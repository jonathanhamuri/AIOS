[BITS 16]
[ORG 0x7C00]

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    mov [boot_drive], dl
    sti

    mov si, msg_start
    call print16

    ; Load 32 sectors (stage2 + kernel) starting at sector 2 to 0x7E00
    mov ah, 0x02
    mov al, 32
    mov ch, 0
    mov cl, 2
    mov dh, 0
    mov dl, [boot_drive]
    mov bx, 0x7E00
    int 0x13
    jc disk_error

    mov si, msg_ok
    call print16
    jmp 0x0000:0x7E00

disk_error:
    mov si, msg_err
    call print16
    jmp $

print16:
    lodsb
    or al, al
    jz .done
    mov ah, 0x0E
    int 0x10
    jmp print16
.done:
    ret

boot_drive db 0
msg_start  db "AIOS loading...", 0x0D, 0x0A, 0
msg_ok     db "Stage 1 OK", 0x0D, 0x0A, 0
msg_err    db "Disk error!", 0x0D, 0x0A, 0

times 510 - ($ - $$) db 0
dw 0xAA55
