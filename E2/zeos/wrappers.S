#include <asm.h>

	
ENTRY(write)
	pushl %ebp
	mov %esp,%ebp
	
	#Salvamos los parametros
	pushl %edx
	pushl %ecx
	pushl %ebx
	
	# Pasar parametros 
	mov 0x08(%ebp), %edx # fd
	mov 0x0c(%ebp), %ecx # buffer
	mov 0x10(%ebp), %ebx # size

	# id syscall write: 4 -> eax
	movl $4, %eax
	
	# Guardar para user stack
	pushl %edx
	pushl %ecx

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
	#recuperamos los registros salvados
	popl %ebx
	popl %ecx
	popl %edx
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
	popl %edx
	popl %ecx
	cmpl $0, %eax
	jge gt_no_error
	
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
	
	movl %eax, errno
	movl -1, %eax

gp_no_error:
	popl %ebp
	ret


ENTRY(fork)
	pushl %ebp
	mov %esp, %ebp
	
	# save for user stack
	pushl %edx
	pushl %ecx

	# sys call n2
	mov $0x2, %eax
	
	pushl $f_ret
	pushl %ebp
	mov %esp, %ebp

	sysenter

f_ret: 
	pop %ebp
	add $4, %esp
	pop %ecx
	pop %edx
	cmpl $0, %eax
	jge f_no_err
		
	mov %eax, errno
	mov $-1, %eax

f_no_err:
	pop %ebp
	ret


