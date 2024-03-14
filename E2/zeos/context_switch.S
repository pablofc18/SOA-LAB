#include <asm.h>

ENTRY(task_switch)
	popl %ebp
	mov %esp, %ebp
	
	# save regs
	pushl %esi 	
	pushl %ebx 	
	pushl %edi

	# parameter
	pushl 0x8(%ebp)	
	# call inner
	call inner_task_switch

	popl %edi
	popl %ebx
	popl %esi
	
	popl %ebp
	ret	

ENTRY(inner_task_switch_ass)
	#store current ebp to pcb.
