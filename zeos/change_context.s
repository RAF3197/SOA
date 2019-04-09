# 1 "change_context.S"
# 1 "<built-in>"
# 1 "<command-line>"
# 31 "<command-line>"
# 1 "/usr/include/stdc-predef.h" 1 3 4
# 32 "<command-line>" 2
# 1 "change_context.S"
# 1 "include/asm.h" 1
# 2 "change_context.S" 2

.globl change_context; .type change_context, @function; .align 0; change_context:
    push %ebp
    movl %esp,%ebp

    movl 0x0c(%ebp),%eax
    movl %ebp,(%eax)
    movl 0x08(%ebp),%esp
    popl %ebp
    ret
