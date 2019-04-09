# 1 "getpid.S"
# 1 "<built-in>"
# 1 "<command-line>"
# 31 "<command-line>"
# 1 "/usr/include/stdc-predef.h" 1 3 4
# 32 "<command-line>" 2
# 1 "getpid.S"
# 1 "include/asm.h" 1
# 2 "getpid.S" 2

.globl getpid; .type getpid, @function; .align 0; getpid:
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
