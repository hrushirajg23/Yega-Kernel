.globl enable_paging

enable_paging:
    movl 4(%esp), %eax
    movl %eax, %cr3
    movl %cr0, %eax
    orl  %0x80000000, %eax
    movl %eax, %cr0
    ret
