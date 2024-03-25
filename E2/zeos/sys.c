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
	// get new page dir and ini dirpagAddr with allocate dir
	allocate_DIR(&(child->task));	
	// [+] allocate antes que get PT [+]
	page_table_entry *childPagTab = get_PT(&(child->task));
	
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
  int q = get_quantum(*current);
 
  
	return zeos_ticks;
}
