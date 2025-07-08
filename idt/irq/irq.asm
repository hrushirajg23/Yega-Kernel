%macro irq_stub 1
irq_stub_%+%1:
    push dword 0

    push dword (32 + %1)

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

    mov eax, esp
    push eax

    call irq_handler

    add esp, 4
    pop gs
    pop fs
    pop es
    pop ds
    popa
    add esp, 8

    iret
%endmacro

extern irq_handler

irq_stub 0
irq_stub 1
irq_stub 2
irq_stub 3
irq_stub 4
irq_stub 5
irq_stub 6
irq_stub 7
irq_stub 8
irq_stub 9
irq_stub 10
irq_stub 11
irq_stub 12
irq_stub 13
irq_stub 14
irq_stub 15

global irq_stub_table

irq_stub_table:
%assign i 0
%rep    16
    dd irq_stub_%+i 
%assign i i+1
%endrep
