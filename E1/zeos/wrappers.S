#include <asm.h>

	
ENTRY(write)
	pushl %ebp
	mov %esp,%ebp
	
	# Guardar para user stack
	pushl %edx
	pushl %ecx

	# Pasar parametros 
	mov 0x08(%ebp), %edx # fd
	mov 0x0c(%ebp), %ecx # buffer
	mov 0x10(%ebp), %ebx # size

	# id syscall write: 4 -> eax
	movl $4, %eax
	
 	# Guardar return address 	
	pushl $write_ret
	# Fake dynamic link
	pushl %ebp
	mov %esp,%ebp

	# Entramos
	sysenter

write_ret:
	# Quitar temporal data
	popl %ebp
	addl $4, %esp
	popl %ecx
	popl %edx
	
	# Miramos si no hay error
	cmpl $0, %eax
	jge write_no_error

	# Si hay error:
	movl %eax, errno # Pasamos valor a errno y asignamos -1 a eax para user
	movl -1, %eax

write_no_error:
	popl %ebp
	ret	

ENTRY(gettime)
	pushl %ebp
	mov %esp,%ebp
	
	pushl %edx
	pushl %ecx
	
	movl $10, %eax
	
	pushl $gt_return
	pushl %ebp
	mov %esp,%ebp

	sysenter

gt_return:
	popl %ebp
	add $4, %esp
	popl %ecx
	popl %edx
	cmpl $0, %eax
	jge gt_no_error
	
	# Si hay error:
	movl %eax, errno
	movl -1, %eax

gt_no_error:
	popl %ebp
	ret

ENTRY(getpid)
	pushl %ebp
	mov %esp,%ebp
	
	pushl %edx
	pushl %ecx
	
	movl $20, %eax
	
	pushl $gp_return
	pushl %ebp
	mov %esp,%ebp

	sysenter

gp_return:
	popl %ebp
	add $4, %esp
	popl %ecx
	popl %edx
	cmpl $0, %eax
	jge gp_no_error
	
	# Si hay error:
  movl %eax, errno
	movl -1, %eax

gp_no_error:
	popl %ebp
	ret

