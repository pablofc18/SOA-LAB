/*
 * interrupt.c -
 */
#include <types.h>
#include <interrupt.h>
#include <segment.h>
#include <hardware.h>
#include <io.h>

#include <zeos_interrupt.h>

Gate idt[IDT_ENTRIES];
Register    idtR;

void keyboard_handler();
void clock_handler();
void pf_handler();

void syscall_handler_sysenter();

void writeMSR(unsigned long msr_val, unsigned long value);
extern int zeos_ticks;
extern struct task_struct *idle_task;
extern struct task_struct *init_task;

char char_map[] =
{
  '\0','\0','1','2','3','4','5','6',
  '7','8','9','0','\'','�','\0','\0',
  'q','w','e','r','t','y','u','i',
  'o','p','`','+','\0','\0','a','s',
  'd','f','g','h','j','k','l','�',
  '\0','�','\0','�','z','x','c','v',
  'b','n','m',',','.','-','\0','*',
  '\0','\0','\0','\0','\0','\0','\0','\0',
  '\0','\0','\0','\0','\0','\0','\0','7',
  '8','9','-','4','5','6','+','1',
  '2','3','0','\0','\0','\0','<','\0',
  '\0','\0','\0','\0','\0','\0','\0','\0',
  '\0','\0'
};

void setInterruptHandler(int vector, void (*handler)(), int maxAccessibleFromPL)
{
  /***********************************************************************/
  /* THE INTERRUPTION GATE FLAGS:                          R1: pg. 5-11  */
  /* ***************************                                         */
  /* flags = x xx 0x110 000 ?????                                        */
  /*         |  |  |                                                     */
  /*         |  |   \ D = Size of gate: 1 = 32 bits; 0 = 16 bits         */
  /*         |   \ DPL = Num. higher PL from which it is accessible      */
  /*          \ P = Segment Present bit                                  */
  /***********************************************************************/
  Word flags = (Word)(maxAccessibleFromPL << 13);
  flags |= 0x8E00;    /* P = 1, D = 1, Type = 1110 (Interrupt Gate) */

  idt[vector].lowOffset       = lowWord((DWord)handler);
  idt[vector].segmentSelector = __KERNEL_CS;
  idt[vector].flags           = flags;
  idt[vector].highOffset      = highWord((DWord)handler);
}

void setTrapHandler(int vector, void (*handler)(), int maxAccessibleFromPL)
{
  /***********************************************************************/
  /* THE TRAP GATE FLAGS:                                  R1: pg. 5-11  */
  /* ********************                                                */
  /* flags = x xx 0x111 000 ?????                                        */
  /*         |  |  |                                                     */
  /*         |  |   \ D = Size of gate: 1 = 32 bits; 0 = 16 bits         */
  /*         |   \ DPL = Num. higher PL from which it is accessible      */
  /*          \ P = Segment Present bit                                  */
  /***********************************************************************/
  Word flags = (Word)(maxAccessibleFromPL << 13);

  //flags |= 0x8F00;    /* P = 1, D = 1, Type = 1111 (Trap Gate) */
  /* Changed to 0x8e00 to convert it to an 'interrupt gate' and so
     the system calls will be thread-safe. */
  flags |= 0x8E00;    /* P = 1, D = 1, Type = 1110 (Interrupt Gate) */

  idt[vector].lowOffset       = lowWord((DWord)handler);
  idt[vector].segmentSelector = __KERNEL_CS;
  idt[vector].flags           = flags;
  idt[vector].highOffset      = highWord((DWord)handler);
}


void setIdt()
{
  /* Program interrups/exception service routines */
  idtR.base  = (DWord)idt;
  idtR.limit = IDT_ENTRIES * sizeof(Gate) - 1;
  
  set_handlers();

  /* ADD INITIALIZATION CODE FOR INTERRUPT VECTOR */

  // a�adimos a idt los handlers de clock y keyboard 
  setInterruptHandler(14, pf_handler, 0);
  setInterruptHandler(32, clock_handler, 0);
  setInterruptHandler(33, keyboard_handler, 0);

  writeMSR(0x174, __KERNEL_CS);
  writeMSR(0x175, INITIAL_ESP);
  writeMSR(0x176, (unsigned long)syscall_handler_sysenter);

  set_idt_reg(&idtR);
}


void keyboard_routine(void) 
{
  unsigned char port_value = inb(0x60);
  if ((port_value & 0x80) != 0x80) {
	char ch = char_map[port_value & 0x7F];
	if (ch == '\0') ch = 'C';
    printc_xy(5,6,ch);
  }  
}


void clock_routine(void)
{
  ++zeos_ticks;
  zeos_show_clock();
  schedule();
}


char *long_to_char(unsigned long num, char* buff)
{
  int i = 0;
  if (num == 0) {
	buff[i++] = '0';
	buff[i] = '\0';
	return buff;
  }

  while (num != 0) {
	int res = num % 16;
	if (res < 10)
	  buff[i++] = res + '0';
	else
	  buff[i++] = res - 10 + 'A';
	num = num/16;
  }

  buff[i] = '\0';
  // reverse the char*
  int start = 0;
  int end = i-1;
  while (start < end) {
	char tmp = buff[start];
	buff[start] = buff[end];
	buff[end] = tmp;
	start++; end--;	
  }
  return buff;
}

void pf_routine(unsigned long param, unsigned long eip) 
{
  char buff[21]; // 20 max digit long + '\0'
  long_to_char(eip, buff);
  printk("\nProcess generate PAGE FAULT exception at: 0x");
  printk(buff);
  while(1);
}
