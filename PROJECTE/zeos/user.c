#include <libc.h>

char buff[24];

int pid;

int lasttime=0.0f;
int frames = 0;
float fps = 0.0f;

void mostrar_fps() {
	gotoxy(50, 0);
	set_color(2,0);
	write(1,"fps: ",4);

	int currenttime=gettime();	
	float tickselapsed=(float)currenttime-(float)lasttime;
	
	float secselapsed=tickselapsed/18.0f;
	fps = (float) frames/secselapsed;
	
	// Se puede o bien poner a cero y actualizar lasttime o bien
	// que siga corriendo valor de fps no se vera afectado
	//frames = 0;
	//lasttime=currenttime;
	
	int i_fps = (int) fps;
	itoa(i_fps,buff);
	write(1,buff,strlen(buff));
}

void show_map() {
	frames++;
	// pintar todo negro para borrar texto
	for (int i = 0; i < 80; ++i) {
		for (int j = 0; j < 25; ++j) {
			gotoxy(i,j);
			char *b=".";
			set_color(0,0);
			write(1,b,1);
		}
	}

	const int rows = 25;
	const int cols = 80;
	set_color(1,0);
	gotoxy(0,0);
	for(int i =0; i < rows; ++i)
	{
		for (int j=0;j<cols;++j)
		{
			if (i==0||i==rows-1||j==0||j==cols-1) {
				char * b = "#";
				set_color(1,0);
				gotoxy(j,i);
				write(1,b,strlen(b));
			}
			else {
				char *b=".";
				set_color(0,3);
				gotoxy(j,i);
				write(1,b,strlen(b));
			}
		}
	} 

	for (int i = 7; i < 12; i++) {
				char * b = "#";
				gotoxy(35,i);
				write(1,b,strlen(b));
  }
  for (int i = 7; i < 12; i++) {
				char * b = "#";
				gotoxy(45,i);
				write(1,b,strlen(b));
		
  }
  for (int j = 35; j < 45; j++) {
				char * b = "#";
				gotoxy(j,7);
				write(1,b,strlen(b));
  }
  for (int j = 35; j < 45; j++) {
				char * b = "#";
				gotoxy(j,11);
				write(1,b,strlen(b));
  }
	mostrar_fps();
}


int __attribute__ ((__section__(".text.main")))
  main(void)
{
    /* Next line, tries to move value 0 to CR3 register. This register is a privileged one, and so it will raise an exception */
     /* __asm__ __volatile__ ("mov %0, %%cr3"::"r" (0) ); */

// PAGE FAULT NORMAL
	/*char*p=0;
	*p='x';*/

// PAGE FAULT COW
	/*char *c = "CCCC";
	if (fork() > 0) {
		int x = 2;
		if (x%2==0) write(1,c,strlen(c));
		exit();
	} else {
		char *b = "HOLA";
		write(1,b,strlen(b));
	}*/

	lasttime = gettime();
	show_map();

  while(1) { 
	  //show_map();
		//mostrar_fps();
	}
}
