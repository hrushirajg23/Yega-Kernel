; define constants needed for Multiboot Header
MBALIGN  equ  1 << 0              ; align modules on page boundaries
MEMINFO  equ  1 << 1              ; give the kernel memory map info
MBFLAGS  equ  MBALIGN | MEMINFO 
MAGIC    equ  0x1BADB002          ; the magic number needed for GRUB
CHECKSUM equ -(MAGIC + MBFLAGS)   ; checksum for validating
                                
; placing the multiboot header in binary
section .multiboot
align 4
	dd MAGIC
	dd MBFLAGS
	dd CHECKSUM

; set up the stack
section .bss
align 16                         
stack_bottom:
resb 16384                        ; reserve 16 KiB for stack space
stack_top:

; entry point of kernel
section .text
global _start:function (_start.end - _start)
_start:
    lea esp, [stack_top]         ; Load stack pointer with address of stack_top

	push ebx                     ; second argument on the stack (addr)
	push eax                     ; first argument on the stack (magic)

	extern kernel_main
	call kernel_main
	
	add esp, 0x8

	cli                          ; clear interrupt flag in case kernel_main returns
	
; halt the CPU 
.hang:	hlt                   
	jmp .hang

; marking the end of the _start
.end: