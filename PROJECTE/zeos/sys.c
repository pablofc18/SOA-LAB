/*
 * sys.c - Syscalls implementation
 */
#include <devices.h>

#include <utils.h>

#include <io.h>

#include <mm.h>

#include <mm_address.h>

#include <sched.h>

#include <p_stats.h>

#include <errno.h>

#include <circular_buffer.h>

#define LECTURA 0
#define ESCRIPTURA 1

void * get_ebp();

int check_fd(int fd, int permissions)
{
  if (fd!=1) return -EBADF; 
  if (permissions!=ESCRIPTURA) return -EACCES; 
  return 0;
}

void user_to_system(void)
{
  update_stats(&(current()->p_stats.user_ticks), &(current()->p_stats.elapsed_total_ticks));
}

void system_to_user(void)
{
  update_stats(&(current()->p_stats.system_ticks), &(current()->p_stats.elapsed_total_ticks));
}

int sys_ni_syscall()
{
	return -ENOSYS; 
}

int sys_getpid()
{
	return current()->PID;
}

int global_PID=1000;

int ret_from_fork()
{
  return 0;
}

int sys_fork(void)
{
  struct list_head *lhcurrent = NULL;
  union task_union *uchild;
  
  /* Any free task_struct? */
  if (list_empty(&freequeue)) return -ENOMEM;

  lhcurrent=list_first(&freequeue);
  
  list_del(lhcurrent);
  
  uchild=(union task_union*)list_head_to_task_struct(lhcurrent);
  
  /* Copy the parent's task struct to child's */
  copy_data(current(), uchild, sizeof(union task_union));
  
  /* new pages dir */
  allocate_DIR((struct task_struct*)uchild);
  
  /* Allocate pages for DATA+STACK */
  int new_ph_pag, pag, i;
  page_table_entry *process_PT = get_PT(&uchild->task);
  for (pag=0; pag<NUM_PAG_DATA; pag++)
  {
    new_ph_pag=alloc_frame();
    if (new_ph_pag!=-1) /* One page allocated */
    {
      set_ss_pag(process_PT, PAG_LOG_INIT_DATA+pag, new_ph_pag);
    }
    else /* No more free pages left. Deallocate everything */
    {
      /* Deallocate allocated pages. Up to pag. */
      for (i=0; i<pag; i++)
      {
        free_frame(get_frame(process_PT, PAG_LOG_INIT_DATA+i));
        del_ss_pag(process_PT, PAG_LOG_INIT_DATA+i);
      }
      /* Deallocate task_struct */
      list_add_tail(lhcurrent, &freequeue);
      
      /* Return error */
      return -EAGAIN; 
    }
  }

  /* Copy parent's SYSTEM and CODE to child. */
  page_table_entry *parent_PT = get_PT(current());
  for (pag=0; pag<NUM_PAG_KERNEL; pag++)
  {
    set_ss_pag(process_PT, pag, get_frame(parent_PT, pag));
  }
  for (pag=0; pag<NUM_PAG_CODE; pag++)
  {
    set_ss_pag(process_PT, PAG_LOG_INIT_CODE+pag, get_frame(parent_PT, PAG_LOG_INIT_CODE+pag));
  }
  /* Copy parent's DATA to child. We will use TOTAL_PAGES-1 as a temp logical page to map to */
  for (pag=NUM_PAG_KERNEL+NUM_PAG_CODE; pag<NUM_PAG_KERNEL+NUM_PAG_CODE+NUM_PAG_DATA; pag++)
  {
    /* Map one child page to parent's address space. */
    set_ss_pag(parent_PT, pag+NUM_PAG_DATA, get_frame(process_PT, pag));
    copy_data((void*)(pag<<12), (void*)((pag+NUM_PAG_DATA)<<12), PAGE_SIZE);
    del_ss_pag(parent_PT, pag+NUM_PAG_DATA);
  }

	// Copy shared memory if available
	for (pag=NUM_PAG_KERNEL+NUM_PAG_CODE+NUM_PAG_DATA; pag<TOTAL_PAGES; pag++)
	{
		if (parent_PT[pag].bits.present) {
			set_ss_pag(process_PT, pag, get_frame(parent_PT, pag));
		}
	}

  /* Deny access to the child's memory space */
  set_cr3(get_DIR(current()));

  uchild->task.PID=++global_PID;
  uchild->task.state=ST_READY;

  int register_ebp;		/* frame pointer */
  /* Map Parent's ebp to child's stack */
  register_ebp = (int) get_ebp();
  register_ebp=(register_ebp - (int)current()) + (int)(uchild);

  uchild->task.register_esp=register_ebp + sizeof(DWord);

  DWord temp_ebp=*(DWord*)register_ebp;
  /* Prepare child stack for context switch */
  uchild->task.register_esp-=sizeof(DWord);
  *(DWord*)(uchild->task.register_esp)=(DWord)&ret_from_fork;
  uchild->task.register_esp-=sizeof(DWord);
  *(DWord*)(uchild->task.register_esp)=temp_ebp;

  /* Set stats to 0 */
  init_stats(&(uchild->task.p_stats));

  /* Queue child process into readyqueue */
  uchild->task.state=ST_READY;
  list_add_tail(&(uchild->task.list), &readyqueue);
  
  return uchild->task.PID;
}

#define TAM_BUFFER 512

int sys_write(int fd, char *buffer, int nbytes) {
char localbuffer [TAM_BUFFER];
int bytes_left;
int ret;

	if ((ret = check_fd(fd, ESCRIPTURA)))
		return ret;
	if (nbytes < 0)
		return -EINVAL;
	if (!access_ok(VERIFY_READ, buffer, nbytes))
		return -EFAULT;
	
	bytes_left = nbytes;
	while (bytes_left > TAM_BUFFER) {
		copy_from_user(buffer, localbuffer, TAM_BUFFER);
		ret = sys_write_console(localbuffer, TAM_BUFFER);
		bytes_left-=ret;
		buffer+=ret;
	}
	if (bytes_left > 0) {
		copy_from_user(buffer, localbuffer,bytes_left);
		ret = sys_write_console(localbuffer, bytes_left);
		bytes_left-=ret;
	}
	return (nbytes-bytes_left);
}


extern int zeos_ticks;

int sys_gettime()
{
  return zeos_ticks;
}

void sys_exit()
{  
  int i;

  page_table_entry *process_PT = get_PT(current());

  // Deallocate all the propietary physical pages
  for (i=0; i<NUM_PAG_DATA; i++)
  {
    free_frame(get_frame(process_PT, PAG_LOG_INIT_DATA+i));
    del_ss_pag(process_PT, PAG_LOG_INIT_DATA+i);
  }
	// deallocate if shared memory
	for (i = PAG_LOG_INIT_DATA+NUM_PAG_DATA; i < TOTAL_PAGES; i++)
	{
		if (process_PT[i].bits.present) {
			free_frame(get_frame(process_PT, i));
			del_ss_pag(process_PT, i);
		}
	}
  
  /* Free task_struct */
  list_add_tail(&(current()->list), &freequeue);
  
  current()->PID=-1;
  
  /* Restarts execution of the next process */
  sched_next_rr();
}

/* System call to force a task switch */
int sys_yield()
{
  force_task_switch();
  return 0;
}

extern int remaining_quantum;

int sys_get_stats(int pid, struct stats *st)
{
  int i;
  
  if (!access_ok(VERIFY_WRITE, st, sizeof(struct stats))) return -EFAULT; 
  
  if (pid<0) return -EINVAL;
  for (i=0; i<NR_TASKS; i++)
  {
    if (task[i].task.PID==pid)
    {
      task[i].task.p_stats.remaining_ticks=remaining_quantum;
      copy_to_user(&(task[i].task.p_stats), st, sizeof(struct stats));
      return 0;
    }
  }
  return -ESRCH; /*ESRCH */
}

extern Byte x;
extern Byte y;

int sys_gotoxy(int xn, int yn)
{	
	// num columns = 80 | num rows = 25
	if (xn < 0 || yn < 0) return -EINVAL;
	// si son positivos mas grandes de 80 o 25 el modulo lo arregla
  // no haria falta comprobar eso (?)
	x = xn % 80;
	y = yn % 25;
	return 0;
}

extern Word color_txt;

int sys_set_color(int fg, int bg)
{
	if (fg < 0 || fg > 15 || bg < 0 || bg > 15) return -EINVAL; 
	Byte fgn = (Byte) (fg);
	Byte bgn = (Byte) (bg);
	color_txt = 0x0000;
	color_txt = (fgn << 4) + bgn;
	color_txt = color_txt << 8;
	return 0;		
}
 
extern CircularBuffer kbd_circularBuffer;

int sys_read(char *b, int maxchars)
{
	if (maxchars < 0 || maxchars >= BUFFER_SIZE) return -EINVAL;
	if (!access_ok(VERIFY_WRITE, b, maxchars)) return -EFAULT;

  int i = 0; 
  char a; 
  while(i < maxchars && circularBufferDequeue(&kbd_circularBuffer,&a)) { 
		copy_to_user(&a,b,sizeof(a));
		++b;
    ++i;
  }
  return i;
}

//if addr correct and empty -> proceed
//else if  addr not possible -> find empty region and proceed
//else if addr NULL -> find empty region and proceed 
// return error if addr is not page aligned
void *sys_shmat(int id, void* addr){
  if(id<0 || id > 9) return -EINVAL;
  if(((unsigned long)addr & 0xfff) != 0) return -EFAULT;      
  unsigned long id_log = (unsigned long)addr>>12;
  page_table_entry * process_pt = get_PT(current());

  if(id_log>= 1024) return -EINVAL;
  if(addr == NULL || process_pt[id_log].bits.present){
    int trobat = 0; 
    for(int i = id_log; i < TOTAL_PAGES-1; ++i){
      if(process_pt[i].bits.present == 0){
        id_log = i;
        trobat = 1;
        break;
      }
    }
    if(!trobat) return -EFAULT;
    set_ss_pag(process_pt, id_log, shared_vector[id].id_frame);
  }else{
    set_ss_pag(process_pt, id_log, shared_vector[id].id_frame);
  } 
  ++shared_vector[id].ref;
  void * result = (void *)(id_log<<12);
  return result;
}

int sys_shmdt(void* addr){
  if(addr == NULL) return -EINVAL; 
  if(((unsigned long)addr & 0xfff) != 0) return -EFAULT;
  
  unsigned long id_log = (unsigned long)addr>>12;
  if(id_log>= 1024) return -EINVAL;

  page_table_entry * process_pt = get_PT(current());

  int i;
  for(i = 0; i < 10; ++i){
    if(get_frame(process_pt, id_log) == shared_vector[i].id_frame){
      --shared_vector[i].ref;
      break;
    }
  }

  if(shared_vector[i].delete && shared_vector[i].ref == 0){
    for(int j = 0; j < 4096; ++j) {
      ((char*)addr)[j] = 0;
    }
  }

  del_ss_pag(process_pt, id_log);
  set_cr3(get_DIR(current()));

  return 0;
}

int sys_shmrm(int id){
  if(id> 9 || id < 0) return -EINVAL;
  shared_vector[id].delete = 1;
  return 0;
}
