#include <libc.h>

char buff[24];

int pid;

int __attribute__ ((__section__(".text.main")))
  main(void)
{
    /* Next line, tries to move value 0 to CR3 register. This register is a privileged one, and so it will raise an exception */
     /* __asm__ __volatile__ ("mov %0, %%cr3"::"r" (0) ); */
  char * msg;
  msg ="\n testeando getpid()\n";
  write(1,msg,strlen(msg)); 
  int miPID=  getpid();
  itoa(miPID, msg);
  if(write(1,msg,strlen(msg) == -1)) perror();
  msg = "\n ---------------\n\n";
  write(1,msg,strlen(msg));
  while(1) {}
}
