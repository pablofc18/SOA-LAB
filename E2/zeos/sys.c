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
int zeos_ticks = 0;
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

extern struct list_head freequeue;

int sys_fork()
{
  int PID=-1;
  // creates the child process
  
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
<<<<<<< HEAD
			return -1;
=======
			return -ENOMEM;
>>>>>>> c922f012377e252d3cb1abd964e02228d103e4d5
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
		set_ss_pag(parentPagTab, page+NUM_PAG_DATA, get_frame(childPagTab, page);
		// shift <<12 for 0x...000 and @ (void*) -> [ej en system.c (linia 98)]
		copy_data((void *) (page<<12), (void *) ((page+NUM_PAG_DATA)<<12), PAGE_SIZE);
		del_ss_pag(parentPagTab, page+NUM_PAG_DATA);	
	}
	
	// flush tlb
	set_cr3(get_DIR(current()));
	
  return PID;
}

void sys_exit()
{  
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
