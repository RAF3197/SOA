#include <asm.h>

ENTRY(sem_signal)
		pushl %ebp
		movl %esp, %ebp
		pushl %ebx
	
		movl 0x08(%ebp), %ebx

		
		lea ret_addr, %eax
		
		pushl %eax
		pushl %ebp
		movl %esp, %ebp	

		movl $23,%eax

		sysenter
ret_addr:		
		pop %ebp
		pop %edx
		pop %ebx
		movl %ebp,%esp
		popl %ebp
		ret

