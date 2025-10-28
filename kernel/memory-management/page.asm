global enable_paging

enable_paging:
    mov     eax, [esp + 4]    ; Get the argument (page directory base)
    mov     cr3, eax          ; Load page directory base address
    mov     eax, cr0
    or      eax, 0x80000000   ; Set PG bit (bit 31) to enable paging
    mov     cr0, eax
    ret

