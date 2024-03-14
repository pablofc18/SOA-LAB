/*
 * libc.c 
 */

#include <libc.h>

#include <types.h>

#include <errno.h>
int errno;

void itoa(int a, char *b)
{
  int i, i1;
  char c;
  
  if (a==0) { b[0]='0'; b[1]=0; return ;}
  
  i=0;
  while (a>0)
  {
    b[i]=(a%10)+'0';
    a=a/10;
    i++;
  }
  
  for (i1=0; i1<i/2; i1++)
  {
    c=b[i1];
    b[i1]=b[i-i1-1];
    b[i-i1-1]=c;
  }
  b[i]=0;
}

int strlen(char *a)
{
  int i;
  
  i=0;
  
  while (a[i]!=0) i++;
  
  return i;
}

void perror() 
{
	char * msg_error = "";

	switch (errno) {
		case -EFAULT:
			msg_error = "Bad address";
			break;
		case -EACCES:
			msg_error = "Permission denied";
			break;
		case -ENOSYS:
			msg_error = "Function not implemented";
			break;
		case -EBADF:
			msg_error = "Bad file descriptor";
			break;
		case -EINVAL:
			msg_error = "Invalid argument";
			break;
	}

	write(1,msg_error,strlen(msg_error));
}
