#include <asm.h>

ENTRY(create_thread)
	pushl %ebp
	movl %esp, %ebp
	movl 0x0c(%ebp), %eax
	movl %ebp, (%eax)
	movl 0x08(%ebp), %esp
	popl %ebp
	ret
