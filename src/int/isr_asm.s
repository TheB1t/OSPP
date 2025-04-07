[bits 32]
global isr_table
extern isr_handler

%macro ISR_NOERRCODE 1
global isr%1
isr%1:
    push dword 0         ; Dummy error code
    push dword %1        ; Interrupt number
    jmp isr_common
%endmacro

%macro ISR_ERRCODE 1
global isr%1
isr%1:
    push dword %1        ; Interrupt number
    jmp isr_common
%endmacro

%assign i 0
%rep 256
    %if (i == 8) || (i == 10) || (i == 11) || (i == 12) || (i == 13) || (i == 14) || (i == 17) || (i == 21)
        ISR_ERRCODE i
    %else
        ISR_NOERRCODE i
    %endif
    %assign i i+1
%endrep

isr_common:
    pusha
    push ds
    push es
    push fs
    push gs

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp
    call isr_handler
    add esp, 4

    pop gs
    pop fs
    pop es
    pop ds
    popa

    add esp, 8
    iret

section .data

%macro ISR_ENTRY 1
    dd isr%1
%endmacro

isr_table:
%assign i 0
%rep 256
    ISR_ENTRY i
    %assign i i+1
%endrep

section .note.GNU-stack