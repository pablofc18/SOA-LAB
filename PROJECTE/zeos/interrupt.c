/*
 * interrupt.c -
 */
#include <types.h>
#include <interrupt.h>
#include <segment.h>
#include <hardware.h>
#include <io.h>

#include <sched.h>

#include <zeos_interrupt.h>

#include <circular_buffer.h>

Gate idt[IDT_ENTRIES];
Register    idtR;

char char_map[] =
{
  '\0','\0','1','2','3','4','5','6',
  '7','8','9','0','\'','¡','\0','\0',
  'q','w','e','r','t','y','u','i',
  'o','p','`','+','\0','\0','a','s',
  'd','f','g','h','j','k','l','ñ',
  '\0','º','\0','ç','z','x','c','v',
  'b','n','m',',','.','-','\0','*',
  '\0','\0','\0','\0','\0','\0','\0','\0',
  '\0','\0','\0','\0','\0','\0','\0','7',
  '8','9','-','4','5','6','+','1',
  '2','3','0','\0','\0','\0','<','\0',
  '\0','\0','\0','\0','\0','\0','\0','\0',
  '\0','\0'
};

int zeos_ticks = 0;

void clock_routine()
{
  zeos_show_clock();
  zeos_ticks ++;
  
  schedule();
}

CircularBuffer kbd_circularBuffer;

void keyboard_routine()
{
  unsigned char c = inb(0x60);
	if (c&0x80) {
		circularBufferEnqueue(&kbd_circularBuffer, char_map[c&0x7F]);	
		if (circularBufferIsFull(&kbd_circularBuffer)) printk("EY");
	}
}

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

void clock_handler();
void keyboard_handler();
void system_call_handler();
void pf_handler();

void setMSR(unsigned long msr_number, unsigned long high, unsigned long low);

void setSysenter()
{
  setMSR(0x174, 0, __KERNEL_CS);
  setMSR(0x175, 0, INITIAL_ESP);
  setMSR(0x176, 0, (unsigned long)system_call_handler);
}

void setIdt()
{
  /* Program interrups/exception service routines */
  idtR.base  = (DWord)idt;
  idtR.limit = IDT_ENTRIES * sizeof(Gate) - 1;
  
  set_handlers();

  /* ADD INITIALIZATION CODE FOR INTERRUPT VECTOR */
  setInterruptHandler(32, clock_handler, 0);
  setInterruptHandler(33, keyboard_handler, 0);
	// ADD FOR PAGE FAULT
	setInterruptHandler(14, pf_handler, 0);

  setSysenter();

  set_idt_reg(&idtR);
}


extern Byte phys_mem[TOTAL_PAGES];

int is_cow_pf(unsigned long log_pg) {
	page_table_entry * pt = get_PT(current());
	if (log_pg >= PAG_LOG_INIT_DATA && log_pg < PAG_LOG_INIT_DATA+NUM_PAG_DATA && pt[log_pg].bits.rw == 0) return 1;
	return 0;
}

void handle_pf_cow(unsigned long log_pg) {
	page_table_entry * pt = get_PT(current());
	unsigned long phys_addr = get_frame(pt, log_pg);
	if (phys_mem[phys_addr] > 1) {
		int nw_phf = alloc_frame();	

		// pag que siempre reemplazaremos momentaneamente (random)
		int pag = PAG_LOG_INIT_DATA+NUM_PAG_DATA+1;	
		page_table_entry tmp;
		tmp.entry = pt[pag].entry;
		pt[pag].entry=0;

		// machacamos esa pag en current
		set_ss_pag(pt, pag, nw_phf);
		// copy data in that new frame
		copy_data((void*)(log_pg<<12),(void*)(pag<<12), PAGE_SIZE);
		// delete previous mapping on data
		del_ss_pag(pt, log_pg);
		// map new phys frame on data	
		set_ss_pag(pt,log_pg,nw_phf);
		// delete previos map to pag
		del_ss_pag(pt, pag); 
		
		// restauramos la pag
		pt[pag].entry = tmp.entry;

		// flush tlb
		set_cr3(get_DIR(current()));
	}
	// change bit rw
	else if (phys_mem[phys_addr] == 1) {
		pt[log_pg].bits.rw = 1;
	}
}

void pf_routine(unsigned long param, unsigned long eip)
{
	unsigned long log_pg = (unsigned long) get_cr2() >> 12;
	if (is_cow_pf(log_pg)) {
		handle_pf_cow(log_pg);
	}
	else {
		printk("Page fault EXCEPTION!!");
		while(1);
	}
}
