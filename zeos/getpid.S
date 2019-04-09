#include <asm.h>

ENTRY(getpid)
    pushl %ebp
    movl %esp,%ebp
    pushl %ecx
    pushl %edx

    lea ret_addr, %eax

    pushl %eax
    pushl %ebp
    movl %esp,%ebp

    movl $20,%eax

    sysenter

ret_addr:
    pop %ebp
    pop %edx
    pop %ecx
    movl %ebp,%esp
    pop %ebp
    ret

