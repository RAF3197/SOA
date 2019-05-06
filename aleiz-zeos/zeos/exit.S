#include <asm.h>

ENTRY(exit)
		pushl %ebp
		movl %esp, %ebp
		pushl %ebx
		pushl %esi
		pushl %ecx
		pushl %edx
		
		lea ret_addr, %eax
		
		pushl %eax
		pushl %ebp
		movl %esp, %ebp	

		movl $1,%eax

		sysenter
ret_addr:		
		pop %ebp
		pop %edx
		pop %edx
		pop %ecx
		pop %esi
		pop %ebx
		movl %ebp,%esp
		pop %ebp
		ret

