# 1 "fork.S"
# 1 "<built-in>"
# 1 "<command-line>"
# 31 "<command-line>"
# 1 "/usr/include/stdc-predef.h" 1 3 4
# 32 "<command-line>" 2
# 1 "fork.S"
# 1 "include/asm.h" 1
# 2 "fork.S" 2

.globl fork; .type fork, @function; .align 0; fork:
    pushl %ebp
    movl %esp,%ebp
    pushl %ecx
    pushl %edx

    lea ret_addr, %eax

    pushl %eax
    pushl %ebp
    movl %esp,%ebp

    movl $2,%eax

    sysenter

ret_addr:
    pop %ebp
    pop %edx
    pop %ecx
    movl %ebp,%esp
    pop %ebp
    ret
