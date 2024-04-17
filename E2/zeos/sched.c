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
// DECLARAMOS init_task (for testing)
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
  ts->state=ST_RUN;
  quantum_ticks=ts->quantum;
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
	
  // ini list head sons/brothers
  INIT_LIST_HEAD(&(ts->sons));
  INIT_LIST_HEAD(&(ts->brothers));

	// for testing init taskswitch
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


int is_child(int pid){
  struct list_head * tmp;
  struct list_head * t;
  list_for_each_safe(tmp,t,&(current()->sons))
    if((list_head_to_task_struct(tmp))->PID == pid) return 1; 
  return 0;
}

int is_blocked(struct task_struct *t){
  if(t->state ==ST_BLOCKED) return 1;   
  return 0;
}

// si el fill de PID == pid existeix a la llista de sons retorna el task struct
// aquest ha d'existir si o si  sino petarà i donarà error.
struct list_head * get_child(int pid){
  struct list_head * tmp;
  struct list_head * t;
  list_for_each_safe(tmp,t,&(current()->sons)){
    if((list_head_to_task_struct(tmp))->PID == pid) return tmp;
  }
  return NULL;
} 

int unblock(int pid){
  if(is_child(pid)){
    struct list_head *t = get_child(pid);
    if(t == NULL) return -1;
    if(is_blocked(list_head_to_task_struct(t))){
      update_process_state_rr(current(),&readyqueue); 
    }else{
      current()->pending_unblocks++;
    }
    return 0;
  }
  return -1;
}

void block(void){
  if(current()->pending_unblocks == 0){
    update_process_state_rr(current(),&blocked); 
    schedule();
  }else{
    current()->pending_unblocks--;
  }
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
  if(quantum_ticks == 0 && !list_empty(&readyqueue)) return 1;
  if(quantum_ticks == 0) quantum_ticks = get_quantum(current());
  return 0;
}

void update_process_state_rr(struct task_struct *t, struct list_head *dst_queue)
{
  struct list_head * tmp = &t->list;
  if(t->state!=ST_RUN) list_del(tmp);
  if(dst_queue!=NULL)
  {
   list_add_tail(tmp, dst_queue);
   if(dst_queue!=&readyqueue) t->state=ST_BLOCKED;
   else t->state=ST_READY;
  }
  else t->state=ST_RUN;
}


void sched_next_rr(void)
{
  struct task_struct *next;
  if(!list_empty(&readyqueue))
  {
    printk("\n el seguent proces sera el primer de la llista ready");
    struct list_head *f = list_first(&readyqueue);
    list_del(f);
    next = list_head_to_task_struct(f);
  }else{
    printk("\n readyqueue esta buida, va idle doncs"); 
    next = idle_task;
  }
  next->state=ST_RUN; 
  quantum_ticks = get_quantum(next);
  task_switch((union task_union*)next);
}


void schedule(){
  update_sched_data_rr();
  if(needs_sched_rr()){
    update_process_state_rr(current(), &readyqueue);
    sched_next_rr();
  }
}

