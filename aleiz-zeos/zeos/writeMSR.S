#include <asm.h>

ENTRY(writeMSR)
	
	push    %ebp
	movl    %esp,%ebp
	movl    0x8(%ebp),%ecx
	movl    0xc(%ebp),%eax
	movl    $0,%edx
	wrmsr
	movl 	%ebp,%esp
	pop    %ebp
    	ret 
