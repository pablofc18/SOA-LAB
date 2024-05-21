#include <libc.h>

char buff[24];

int pid;

float lasttime=0.0f;
int frames = 0;
float fps = 0.0f;

void mostrar_fps() {
	gotoxy(50, 0);
	set_color(2,0);
	write(1,"fps: ",4);

	float currenttime=gettime();	
	gotoxy(30, 0);
if (currenttime>0.00) write(1,"T",1);
	itoa(currenttime,buff);
	write(1,buff,strlen(buff));
	float tickselapsed=currenttime-lasttime;
	
	float secselapsed=tickselapsed/18.0f;
	if (secselapsed>=1.0) {
	fps = (float) frames/secselapsed;
	
	frames = 0;
	lasttime=currenttime;

	itoa(fps,buff);
	write(1,buff,strlen(buff));
	}
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



	// se dibuja un frame antes del while(1)
		show_map();

  while(1) { 
		// se incrementa cada iteracion un frame nuevo
		//frames++;

		// procesar movimientos

		//mostrar_fps();
		//frames = 0;
	}
}
