#include <asm.h>

ENTRY(task_switch)
	pushl %ebp
	mov %esp, %ebp
	
	# save regs
	pushl %esi 	
	pushl %ebx 	
	pushl %edi

	# parameter
	pushl 0x8(%ebp)	
	# call inner
	call inner_task_switch

	# quitar param
	add $0x4, %esp
	# restore regs
	popl %edi
	popl %ebx
	popl %esi
	
	popl %ebp
	ret	


ENTRY(inner_task_switch_ass)
	#mov %ebp, 0x4(%esp)
	mov 0x4(%esp), %ebx
	mov %ebp, (%ebx)
	mov 0x8(%esp), %esp
	popl %ebp
	ret
