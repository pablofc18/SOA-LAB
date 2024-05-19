#include <libc.h>

char buff[24];

int pid;

int frames = 0;
float calcular_fps(float tm) {
	return (float) (1/(tm));
}

void mostrar_fps(float bef, float aft) {
	gotoxy(50, 0);
	set_color(2,0);
	write(1,"fps: ",4);
	
	float a = bef/18.0;	
	float b = aft/18.0;

	float fps = calcular_fps(b-a);
	itoa(fps,buff);
	write(1,buff,strlen(buff));
}


int __attribute__ ((__section__(".text.main")))
  main(void)
{
    /* Next line, tries to move value 0 to CR3 register. This register is a privileged one, and so it will raise an exception */
     /* __asm__ __volatile__ ("mov %0, %%cr3"::"r" (0) ); */

	float bef=0, aft=0;
	bef=gettime();

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

	aft = gettime();
	mostrar_fps(bef,aft);
	

  while(1) { 
		bef = gettime();

		// procesar movimientos

		aft = gettime();
		mostrar_fps(bef, aft);
	}
}
