/*
 * sched.c - initializes struct for task 0 anda task 1
 */

#include <sched.h>
#include <mm.h>
#include <io.h>

union task_union task[NR_TASKS]
  __attribute__((__section__(".data.task")));

#if 0
struct task_struct *list_head_to_task_struct(struct list_head *l)
{
  return list_entry( l, struct task_struct, list);
}
#endif

struct task_struct *list_head_to_task_struct(struct list_head *l)
{
  unsigned long addr = (unsigned long) l;
  addr = addr & 0xFFFFF000;
  return (struct task_struct *) addr;	 
}

extern struct list_head blocked;

// DECLARAMOS free y ready queue
struct list_head freequeue;
struct list_head readyqueue;

// DECLARAMOS idle_task
struct task_struct *idle_task;

/* get_DIR - Returns the Page Directory address for task 't' */
page_table_entry * get_DIR (struct task_struct *t) 
{
	return t->dir_pages_baseAddr;
}

/* get_PT - Returns the Page Table address for task 't' */
page_table_entry * get_PT (struct task_struct *t) 
{
	return (page_table_entry *)(((unsigned int)(t->dir_pages_baseAddr->bits.pbase_addr))<<12);
}


int allocate_DIR(struct task_struct *t) 
{
	int pos;

	pos = ((int)t-(int)task)/sizeof(union task_union);

	t->dir_pages_baseAddr = (page_table_entry*) &dir_pages[pos]; 

	return 1;
}

void cpu_idle(void)
{
	__asm__ __volatile__("sti": : :"memory");

	while(1)
	{
	;
	}
}

void init_idle (void)
{
	// first task struct available 
	struct list_head *lh = list_first(&freequeue);
	// delete from fq
	list_del(lh);	
	// Get ts
	struct task_struct *ts = list_head_to_task_struct(lh);
	// pid = 0
	ts->PID = 0;
	// ini dir_pages_baseAddr
	allocate_DIR(ts);
	// task_struct -> task_union
	union task_union *tu = (union task_union*) ts;
	// store in the stack the @ cpu_idle function
	tu->stack[KERNEL_STACK_SIZE-2] = (unsigned long) cpu_idle;
	// store in the stack the initial value of EBP (0 i. e.)
	tu->stack[KERNEL_STACK_SIZE-1] = (unsigned long) 0;
	// pos of stack (in a new field of its task_struct) where ini value of ebp
	tu->task.kernel_esp = &(tu->stack[KERNEL_STACK_SIZE-1]);
	// initialize idle_task (task struct)
	idle_task = ts;

	// es un -> while(1); // nunca sale a user, solo sistema, hay que preparar para q se ejecute contexto de ejecucion, pila de sistema para q idle entre en cpu y qno permita ir a usuario. (Cima pila sistema haya un ebp que pueda recuperar y una dir de retorno a su codigo). Pila sistema no hay context HW SW, solo info minima ebp y @RET. Crear a mano pila <-. ((((Añadir task struct kernel_esp)))). EBP -> 0. @RET -> dir funcion codigo idle cpu_idle (en codigo del kernel). IDLE NUNCA EN LA COLA DE READY. Puntero global a su task union. 
}

void init_task1(void)
{
	// first task struct available 
	struct list_head *lh = list_first(&freequeue);
	// delete from fq
	list_del(lh);	
	// Get ts
	struct task_struct *ts = list_head_to_task_struct(lh);
	// pid = 1
	ts->PID = 1;
	// ini dir_pages_baseAddr	
	allocate_DIR(ts);
	// ini address spaces
	set_user_pages(ts);	
	// TSS -> new_task system stack
	union task_union *tu = (union task_union *) ts;
	tss.esp0 = KERNEL_ESP(tu);
	// msr 0x175 -> stack act proc
	writeMSR(0x175, tss.esp0);
	// page dir -> cur page dir in sys
	set_cr3(ts->dir_pages_baseAddr);
	
	// Codigo para crear el proceso a mano, primer proceso de user, tabla pàg proceso: Definir como es ((mm_addres.h)). setuserpages (crea primera tabla pags, llamarla). Prepararlo para indicar que es current(): cr3 -> tabla pag de este proceso. tss.esp0 -> pila sistema de task1 cima. msr 0x175-> pila sistema del proc actual. TASK1 NO SE ENCOLA EN READY QUEUE. 
}

// pila sistema del proc actual: current devuelve task struct pointer. (pila sist no definida en task struct). 
// Task struct a task union -> cast indirecto. struct task struct pointer p -> (union task union *)p)->stack

void init_sched()
{
  // ini structures 
	// Ready Queue ini empty
	INIT_LIST_HEAD(&readyqueue);
	
	// Free Queue add all tasks unions/structs
	INIT_LIST_HEAD(&freequeue);
	for(int i = 0; i < NR_TASKS; ++i) {
		list_add(&(task[i].task.list), &freequeue);		
	}
}

struct task_struct* current()
{
  int ret_value;
  
  __asm__ __volatile__(
  	"movl %%esp, %0"
	: "=g" (ret_value)
  );
  return (struct task_struct*)(ret_value&0xfffff000);
}

