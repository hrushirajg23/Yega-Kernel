global enable_paging

enable_paging:
    mov     eax, [esp + 4]    ; Get the argument (page directory base)
    mov     cr3, eax          ; Load page directory base address
    mov     eax, cr0
    or      eax, 0x80000000   ; Set PG bit (bit 31) to enable paging
    mov     cr0, eax
    jmp short .flush; short jump to flush pipeline
    .flush:
        ; execution continues here with paging active
    ret

; /*
; explanation of above flushing 

; Before paging:

; CPU fetches instructions directly using physical addresses = linear addresses.

; The instruction fetch pipeline (prefetch queue) may already contain several bytes ahead of the current instruction.


; After paging:
; When you set CR0.PG = 1,
; the CPU does not immediately discard prefetched instructions from its internal pipeline.

; This means:

; The CPU may have already fetched several bytes ahead using the old (non-paged) mapping.

; It could continue executing those bytes before applying the new page table translation,
; which might be inconsistent or invalid.

; That can cause:

; Instruction fetch fault

; Jump to unmapped addresses

; Undefined behavior, depending on timing and CPU generation.

; This short jump makes the CPU:

; Discard any prefetched instructions.

; Reload the instruction stream using the new address translation mechanism.

; Start fetching from label .flush: using paged addressing.

; jmp mechanism:

; When a jmp instruction executes:

; The CPU flushes the instruction queue (prefetch buffer).

; It refills it from the target address of the jump — and now that paging is active, this fetch uses the new translation.

; */

global confirm_paging

confirm_paging:
    mov     eax, cr0      ; Read control register CR0 into EAX
    ret                   ; Return CR0 value in EAX

