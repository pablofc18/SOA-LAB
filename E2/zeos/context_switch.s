# 0 "context_switch.S"
# 0 "<built-in>"
# 0 "<command-line>"
# 1 "/usr/include/stdc-predef.h" 1 3 4
# 0 "<command-line>" 2
# 1 "context_switch.S"
# 1 "include/asm.h" 1
# 2 "context_switch.S" 2

.globl task_switch; .type task_switch, @function; .align 0; task_switch:
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


.globl inner_task_switch_ass; .type inner_task_switch_ass, @function; .align 0; inner_task_switch_ass:
 #mov %ebp, 0x4(%esp)
 mov 0x4(%esp), %ebx
 mov %ebp, (%ebx)
 mov 0x8(%esp), %esp
 popl %ebp
 ret
