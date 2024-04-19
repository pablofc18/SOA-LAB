#include <libc.h>
#include <errno.h>

char buff[24];

int pid;

int __attribute__ ((__section__(".text.main")))
  main(void)
{
    /* Next line, tries to move value 0 to CR3 register. This register is a privileged one, and so it will raise an exception */
     /* __asm__ __volatile__ ("mov %0, %%cr3"::"r" (0) ); */


/*
	/// TESTING ///

  int pid = fork();
	if (pid == 0) {
		char * text= "hola hijo";
		write(1,text,strlen(text));
  	char * msg = "";
  	int pid = getpid();
	 	itoa(pid,msg);
		write(1,msg,strlen(msg));
		block();
		exit();
	}
	else if (pid > 0) {
		char * p = "hola padre";
		write(1,p,strlen(p));
  	char * msg = "";
  	int pid = getpid();
	 	itoa(pid,msg);
		write(1,msg,strlen(msg));
		msg = "desbloqueamos hijo pid 1001";
		unblock(1001);
		write(1,msg,strlen(msg));
	}
*/

  while(1) { }
}
