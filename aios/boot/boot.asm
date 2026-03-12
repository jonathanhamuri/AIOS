[BITS 16]
[ORG 0x7C00]

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti

    ; Load Stage 2 from sector 2 into memory at 0x7E00
    mov ah, 0x02        ; BIOS read sectors function
    mov al, 4           ; Read 2 sectors
    mov ch, 0           ; Cylinder 0
    mov cl, 2           ; Start at sector 2 (sector 1 = us)
    mov dh, 0           ; Head 0
    mov bx, 0x7E00      ; Load into address 0x7E00
    int 0x13            ; BIOS disk interrupt
    jc disk_error       ; Jump if carry flag set (error)

    ; Jump to Stage 2
    jmp 0x0000:0x7E00

disk_error:
    mov si, err_msg
    call print_string
    jmp $

print_string:
    lodsb
    or al, al
    jz .done
    mov ah, 0x0E
    int 0x10
    jmp print_string
.done:
    ret

msg     db "AIOS Stage 1 OK", 0x0D, 0x0A, 0
err_msg db "Disk read error!", 0x0D, 0x0A, 0

times 510 - ($ - $$) db 0
dw 0xAA55
