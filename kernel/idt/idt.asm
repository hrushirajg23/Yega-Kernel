; idt_asm.asm
; Low-level exception handlers (Linux 0.11 style)

BITS 32

global divide_error, debug, nmi, int3, overflow, bounds, invalid_op
global device_not_available, double_fault, coprocessor_segment_overrun
global invalid_TSS, segment_not_present, stack_segment
global general_protection, coprocessor_error, reserved

extern do_divide_error
extern do_int3
extern do_nmi
extern do_overflow
extern do_bounds
extern do_invalid_op
extern do_device_not_available
extern do_coprocessor_segment_overrun
extern do_reserved
extern do_coprocessor_error
extern do_double_fault
extern do_invalid_TSS
extern do_segment_not_present
extern do_stack_segment
extern do_general_protection

; -------------------------
; Exceptions without error code
; -------------------------

divide_error:
    push dword do_divide_error

no_error_code:
    xchg eax, [esp]
    push ebx
    push ecx
    push edx
    push edi
    push esi
    push ebp
    push ds
    push es
    push fs
    push dword 0              ; fake error code
    lea edx, [esp + 44]
    push edx
    mov edx, 0x10             ; kernel data selector
    mov ds, dx
    mov es, dx
    mov fs, dx
    call eax
    add esp, 8
    pop fs
    pop es
    pop ds
    pop ebp
    pop esi
    pop edi
    pop edx
    pop ecx
    pop ebx
    pop eax
    iret

debug:
    push dword do_int3
    jmp no_error_code

nmi:
    push dword do_nmi
    jmp no_error_code

int3:
    push dword do_int3
    jmp no_error_code

overflow:
    push dword do_overflow
    jmp no_error_code

bounds:
    push dword do_bounds
    jmp no_error_code

invalid_op:
    push dword do_invalid_op
    jmp no_error_code

; -------------------------
; Math / coprocessor
; -------------------------

global device_not_available

extern current
extern last_task_used_math
extern math_state_restore


math_emulate:
    pop eax
    push dword do_device_not_available
    jmp no_error_code
; Device Not Available (#NM, vector 7)
; Lazy FPU handling (Linux 0.11 style)
device_not_available:
    push eax

    mov eax, cr0
    bt eax, 2                  ; test EM (math emulation) bit
    jc math_emulate

    clts                       ; clear TS so FPU can be used

    mov eax, [current]
    cmp eax, [last_task_used_math]
    je .done                   ; shouldn't happen really

    push ecx
    push edx
    push ds

    mov eax, 0x10              ; kernel data selector
    mov ds, ax

    call math_state_restore

    pop ds
    pop edx
    pop ecx

.done:
    pop eax
    iret


coprocessor_segment_overrun:
    push dword do_coprocessor_segment_overrun
    jmp no_error_code

reserved:
    push dword do_reserved
    jmp no_error_code

coprocessor_error:
    push dword do_coprocessor_error
    jmp no_error_code

; -------------------------
; Exceptions WITH error code
; -------------------------

double_fault:
    push dword do_double_fault

error_code:
    xchg eax, [esp + 4]        ; error code <-> eax
    xchg ebx, [esp]            ; handler <-> ebx
    push ecx
    push edx
    push edi
    push esi
    push ebp
    push ds
    push es
    push fs
    push eax                  ; error code
    lea eax, [esp + 44]
    push eax
    mov eax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    call ebx
    add esp, 8
    pop fs
    pop es
    pop ds
    pop ebp
    pop esi
    pop edi
    pop edx
    pop ecx
    pop ebx
    pop eax
    iret

invalid_TSS:
    push dword do_invalid_TSS
    jmp error_code

segment_not_present:
    push dword do_segment_not_present
    jmp error_code

stack_segment:
    push dword do_stack_segment
    jmp error_code

general_protection:
    push dword do_general_protection
    jmp error_code
