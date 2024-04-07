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

int quantum_ticks;

// DECLARAMOS idle_task
struct task_struct *idle_task;
// DECLARAMOS init_task
struct task_struct *init_task;

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
	ts->quantum = QUANTUM;
	// ini dir_pages_baseAddr
	allocate_DIR(ts);
	// task_struct -> task_union
	union task_union *tu = (union task_union*) ts;
	// store in the stack the @ cpu_idle function
	tu->stack[KERNEL_STACK_SIZE-1] = (unsigned long) cpu_idle;
	// store in the stack the initial value of EBP (0 i. e.)
	tu->stack[KERNEL_STACK_SIZE-2] = (unsigned long) 0;
	// pos of stack (in a new field of its task_struct) where ini value of ebp
	tu->task.kernel_esp = (unsigned long) &(tu->stack[KERNEL_STACK_SIZE-2]);
	// initialize idle_task (task struct)
	idle_task = ts;

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
	ts->quantum = QUANTUM;
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
	
	// testing init taskswitch
	init_task = ts;
}


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


void inner_task_switch_ass(unsigned long *currkernesp, unsigned long newkernesp);

void inner_task_switch(union task_union*t)
{
	tss.esp0 = KERNEL_ESP(t);
  writeMSR(0x175, tss.esp0);
	set_cr3(get_DIR(&(t->task)));
	// dos params -> curr kern esp | new kern esp
	unsigned long *curkeresp = &(current()->kernel_esp);
	unsigned long newkeresp = t->task.kernel_esp;
	inner_task_switch_ass(curkeresp, newkeresp);	
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
int get_quantum (struct task_struct *t)
{
  //printk("al get quantum tambe entro");
  return t->quantum;
}

void set_quantum(struct task_struct *t,int new_quantum)
{
  t->quantum = new_quantum;
}

void update_sched_data_rr()
{
  --quantum_ticks;
}

int needs_sched_rr()
{
  /*int quantum = quantum_ticks;
  char * msg = " ";
  itoa(msg,quantum);
  printk(msg);*/
  if(quantum_ticks > 0)  return 0;
  if(list_empty(&readyqueue)){
    quantum_ticks = get_quantum(current());
    return 0;
  }
  return 1;
}

void update_process_state_rr(struct task_struct *t, struct list_head *dst_queue)
{
  struct list_head * tmp = &t->list;
  if(!(tmp->prev == NULL && tmp->next == NULL))
  {
    list_del(tmp);
  }
  if(dst_queue) list_add_tail(tmp, dst_queue);
}

void sched_next_rr(void)
{
  struct task_struct *next;
  if(!list_empty(&readyqueue))
  {
    printk("\n el seguent proces sera el primer de la llista ready");
    struct list_head *lf = list_first(&readyqueue);
    list_del(lf);
    next = list_head_to_task_struct(lf);
  }else{
    printk("\n readyqueue esta buida, va idle doncs"); 
    next = idle_task;
  }
  quantum_ticks = get_quantum(next);
  task_switch(next);
}

void schedule();

void schedule(){
  /*int pid = current()->PID;
  char * msg;
  itoa(msg, pid);
  printk(msg);*/
  update_sched_data_rr();
  if(needs_sched_rr()){
    //printk("entro al if ");
    update_process_state_rr(current(), &readyqueue);
    sched_next_rr();
  }
}
