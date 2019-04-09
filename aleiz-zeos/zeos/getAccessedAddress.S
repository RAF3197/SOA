#include <asm.h>

ENTRY(getAccessedAddress)
	pushl %ebp
	movl %esp, %ebp
	movl %cr2, %eax
	movl %ebp, %esp
	popl %ebp
	ret
