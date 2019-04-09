# 1 "writeMSR.S"
# 1 "<built-in>"
# 1 "<command-line>"
# 31 "<command-line>"
# 1 "/usr/include/stdc-predef.h" 1 3 4
# 32 "<command-line>" 2
# 1 "writeMSR.S"
# 1 "include/asm.h" 1
# 2 "writeMSR.S" 2

.globl writeMSR; .type writeMSR, @function; .align 0; writeMSR:

 push %ebp
 movl %esp,%ebp
 movl 0x8(%ebp),%ecx
 movl 0xc(%ebp),%eax
 movl $0,%edx
 wrmsr
 movl %ebp,%esp
 pop %ebp
     ret
