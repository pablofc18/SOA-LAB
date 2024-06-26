/*
 * sys.c - Syscalls implementation
 */
#include <devices.h>

#include <utils.h>

#include <io.h>

#include <mm.h>

#include <mm_address.h>

#include <sched.h>

#include <errno.h>

#define LECTURA 0
#define ESCRIPTURA 1

// Start from 1001 the first one (in fork)
int global_pids = 1000;

int zeos_ticks = 0;
extern struct list_head blocked;

int check_fd(int fd, int permissions)
{
  if (fd!=1) return -9; /*EBADF*/
  if (permissions!=ESCRIPTURA) return -13; /*EACCES*/
  return 0;
}

int sys_ni_syscall()
{
	return -38; /*ENOSYS*/
}

int sys_getpid()
{
	return current()->PID;
}

extern struct list_head freequeue, readyqueue;

int ret_from_fork() { return 0; }

int sys_fork()
{
	// check free free queue if no space error ENOMEM
	if (list_empty(&freequeue)) return -ENOMEM;	
	// get first elem
	struct list_head *lh = list_first(&freequeue);
	list_del(lh);	

	// inherit the parent's task union
	struct task_struct *ts = list_head_to_task_struct(lh);
	union task_union *child = (union task_union*) ts;
	copy_data(current(), child, sizeof(union task_union));

	// new pages dir por the child
	allocate_DIR(&(child->task));	

	// allocate pages for data+stack if not enough ERROR
	page_table_entry *childPagTab = get_PT(&(child->task));
	int page;
	for(page = 0; page < NUM_PAG_DATA; ++page) { // iterate w data
		int child_ff = alloc_frame(); 
		if (child_ff != -1) { // if this is true (!=-1) map page
			set_ss_pag(childPagTab, PAG_LOG_INIT_DATA+page, child_ff);
		}
		else { // deallocate everything, no space
			for(int i = 0; i < page; ++i) {
				// free all the frames we allocated
				free_frame(get_frame(childPagTab, PAG_LOG_INIT_DATA+i));
				// remove the mapping we done allocated
				del_ss_pag(childPagTab, PAG_LOG_INIT_DATA+i);
			}				
			// put the ts again on the free queue
			list_add_tail(lh, &freequeue);
			// return error 
			return -ENOMEM;
		}
	}

	// copy system pages from the parent
	page_table_entry *parentPagTab = get_PT(current());
	for(page = 0; page < NUM_PAG_KERNEL; ++page) {
		// pages from 0 to NUM_PAG_KERNEL
		set_ss_pag(childPagTab, page, get_frame(parentPagTab, page));
	}
	// copy code pages from parent
	for(page = 0; page < NUM_PAG_CODE; ++page) {
		// pages from PAG_LOG_INIT_CODE to PAG_LOG_INIT_CODE + NUM_PAGE_CODE
		set_ss_pag(childPagTab, PAG_LOG_INIT_CODE+page, get_frame(parentPagTab, PAG_LOG_INIT_CODE+page));
	}
	// copy data from parent, we need tmp logical page
	for(page = NUM_PAG_KERNEL+NUM_PAG_CODE; page < NUM_PAG_KERNEL+NUM_PAG_CODE+NUM_PAG_DATA; ++page) {
		set_ss_pag(parentPagTab, page+NUM_PAG_DATA, get_frame(childPagTab, page));
		// shift <<12 for 0x...000 and @ (void*) -> [ej en system.c (linia 98)]
		copy_data((void *) (page<<12), (void *) ((page+NUM_PAG_DATA)<<12), PAGE_SIZE);
		del_ss_pag(parentPagTab, page+NUM_PAG_DATA);	
	}
	
	// flush tlb
	set_cr3(get_DIR(current()));
	
	// assign new PID
  child->task.PID = ++global_pids;

	// change regs for the ret of the proc
	// sw and hw context already saved in stack
	// @RET KERNEL_STACK_SIZE - 18 
	// ebp KERNEL_STACK_SIZE - 19 
	child->stack[KERNEL_STACK_SIZE-18] = (unsigned long) &ret_from_fork;
	child->stack[KERNEL_STACK_SIZE-19] = 0;
	// kernel_esp also needs to be modified
	child->task.kernel_esp = (unsigned long) &(child->stack[KERNEL_STACK_SIZE-19]);

	// child in ready queue 
	child->task.state = ST_READY;
	list_add_tail(&(child->task.list), &readyqueue);

  // ini list children of child
  INIT_LIST_HEAD(&(child->task.sons));
	// add to sons list (children list of each process)
	list_add(&(child->task.brothers), &(current()->sons));
	// and initialize pointer to its father
	child->task.pParent = current();

  return child->task.PID;
}


extern struct task_struct * idle_task;

void sys_exit()
{
  struct task_struct *t = current();
	// remove curr proc from parents list and move any alive children from cur to idle
	struct list_head * p;
	struct list_head * p2;
	list_for_each_safe(p,p2,&(t->pParent->sons))
		if (list_head_to_task_struct(p)->PID == current()->PID) list_del(p);
	list_for_each_safe(p,p2,&(current()->sons)) {
		list_del(p);
		list_add_tail(p, &(idle_task->sons));
	}
	
  page_table_entry *t_PT = get_PT(t);
  for(int i = 0; i<NUM_PAG_DATA; ++i){
    free_frame(get_frame(t_PT,i+PAG_LOG_INIT_DATA));
    del_ss_pag(t_PT,i+PAG_LOG_INIT_DATA);
  }
  t->PID = -1;
  t->dir_pages_baseAddr = NULL;
  list_del(&t->list);
	list_add_tail(&(t->list), &freequeue);
  sched_next_rr();
}

int is_child(int pid){
  struct list_head * tmp;
  struct list_head * t;
  list_for_each_safe(tmp,t,&current()->sons)
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
  list_for_each_safe(tmp,t,&current()->sons){
    if((list_head_to_task_struct(tmp))->PID == pid) return tmp;
  }
  return NULL;
} 

int sys_unblock(int pid){
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

void sys_block(void){
  if(current()->pending_unblocks == 0){
    update_process_state_rr(current(),&blocked); 
    schedule();
  }else{
    current()->pending_unblocks--;
  }
}


#define TAMBUFF 512
char buff[TAMBUFF];
int sys_write(int fd, char * buffer, int size)
{
	// check parameters
	int fd_error = check_fd(fd, ESCRIPTURA);
	if (fd_error) return fd_error;
	if (!access_ok(VERIFY_WRITE, buffer, size)) return -EFAULT;
	if (size < 0) return -EINVAL;
	
	int bytes_left = size;
	int written_bytes;
	while (bytes_left > TAMBUFF) {
		copy_from_user(buffer, buff, TAMBUFF);
		written_bytes = sys_write_console(buff, TAMBUFF);
		bytes_left -= written_bytes;
		buffer += written_bytes;
	}
	
	// bytes restantes:
	if (bytes_left > 0) {
		copy_from_user(buffer, buff, bytes_left);
		written_bytes = sys_write_console(buff, bytes_left);
		bytes_left -= written_bytes;
	}

	return size-bytes_left;
} 

int sys_gettime(void){
	return zeos_ticks;
}
