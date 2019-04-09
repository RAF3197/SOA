# 1 "write.S"
# 1 "<built-in>"
# 1 "<command-line>"
# 31 "<command-line>"
# 1 "/usr/include/stdc-predef.h" 1 3 4
# 32 "<command-line>" 2
# 1 "write.S"
# 1 "include/asm.h" 1
# 2 "write.S" 2

.globl write; .type write, @function; .align 0; write:
    pushl %ebp
    movl %esp,%ebp
    pushl %ebx
    pushl %ecx
    pushl %edx

    movl 0x08(%ebp),%ebx
    movl 0x0c(%ebp),%ecx
    movl 0x10(%ebp),%edx

    lea ret_addr, %eax

    pushl %eax
    pushl %ebp
    movl %esp,%ebp
    movl $4,%eax

    sysenter
ret_addr:
    pop %ebp
    pop %edx
    pop %ecx
    pop %ebx
    movl %ebp,%esp
    popl %ebp
    ret
