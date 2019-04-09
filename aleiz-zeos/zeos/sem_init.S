#include <asm.h>

ENTRY(sem_init)
		pushl %ebp
		movl %esp, %ebp
		pushl %ebx
		pushl %ecx
		
	
		movl 0x08(%ebp), %ebx
 		movl 0x0c(%ebp), %ecx
		

		
		lea ret_addr, %eax
		
		pushl %eax
		pushl %ebp
		movl %esp, %ebp	

		movl $21,%eax

		sysenter
ret_addr:		
		pop %ebp
		pop %edx
		pop %ecx
		pop %ebx
		movl %ebp,%esp
		popl %ebp
		ret

