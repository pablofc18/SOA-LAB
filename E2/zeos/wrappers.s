# 0 "wrappers.S"
# 0 "<built-in>"
# 0 "<command-line>"
# 1 "/usr/include/stdc-predef.h" 1 3 4
# 0 "<command-line>" 2
# 1 "wrappers.S"
# 1 "include/asm.h" 1
# 2 "wrappers.S" 2


.globl write; .type write, @function; .align 0; write:
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

.globl gettime; .type gettime, @function; .align 0; gettime:
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

.globl getpid; .type getpid, @function; .align 0; getpid:
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


.globl fork; .type fork, @function; .align 0; fork:
 pushl %ebp
 mov %esp, %ebp

 # save for user stack
 pushl %edx
 pushl %ecx

 # sys call n2
 mov $0x2, %eax

 # fake dyn link
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

 # errno
 mov %eax, errno
 mov $-1, %eax

f_no_err:
 pop %ebp
 ret
