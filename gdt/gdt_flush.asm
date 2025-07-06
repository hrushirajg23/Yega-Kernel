global gdt_flush

gdt_flush:
    JMP 0x08:.flush_cs

.flush_cs:
    MOV AX, 0x10
    MOV DS, AX
    MOV ES, AX
    MOV FS, AX
    MOV GS, AX
    MOV SS, AX
    RET

