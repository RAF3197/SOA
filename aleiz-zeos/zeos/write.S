#include <asm.h>

ENTRY(write)
		pushl %ebp
		movl %esp, %ebp
		pushl %ebx
		pushl %ecx
		pushl %edx
	
		movl 0x08(%ebp), %ebx
 		movl 0x0c(%ebp), %ecx
		movl 0x10(%ebp), %edx

		
		lea ret_addr, %eax
		
		pushl %eax
		pushl %ebp
		movl %esp, %ebp	

		movl $4,%eax

		sysenter
ret_addr:		
		pop %ebp
		pop %edx
		pop %edx
		pop %ecx
		pop %ebx
		movl %ebp,%esp
		popl %ebp
		ret


