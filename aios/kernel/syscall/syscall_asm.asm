[BITS 32]
global syscall_isr
extern syscall_dispatch

syscall_isr:
    pusha
    push edx
    push ecx
    push ebx
    push eax
    call syscall_dispatch
    add esp, 16
    mov [esp+28], eax   ; store return val into pusha's eax slot
    popa
    iret
