/* i8259.c - Functions to interact with the 8259 interrupt controller
 * vim:ts=4 noexpandtab
 */

#include "i8259.h"
#include "lib.h"

/* Interrupt masks to determine which interrupts
 * are enabled and disabled */
uint8_t master_mask; /* IRQs 0-7 */
uint8_t slave_mask; /* IRQs 8-15 */

/* Initialize the 8259 PIC */
/*
input: none
output: none
effect: initializes the PIC
*/
void
i8259_init(void)
{
	master_mask = inb(MASTER_8259_DATA);	//save state of the master pic
	slave_mask = inb(SLAVE_8259_DATA);		//save state of the slave pic
	
	outb(ICW1, MASTER_8259_CTRL);			//sends 0x11 to master pic, which initializes it in cascade mode
	outb(ICW1,SLAVE_8259_CTRL);				//sends 0x11 to slave pic, which initializes it in cascade mode
	
	outb(ICW2_MASTER, MASTER_8259_DATA);	//vector offset of the master
	outb(ICW2_SLAVE, SLAVE_8259_DATA);		//vector offset of the slave
	
	outb(ICW3_MASTER, MASTER_8259_DATA); 	//lets master pic know that the slave pic is located at IRQ2
	outb(ICW3_SLAVE, SLAVE_8259_DATA);		//lets slave pic know its cascade number
	
	outb(ICW4,MASTER_8259_DATA); 			//additional information for pic (8086 mode)
	outb(ICW4, SLAVE_8259_DATA);			//^
	
	outb(master_mask, MASTER_8259_DATA);	//restore the state of the master pic
	outb(slave_mask, SLAVE_8259_DATA);		//restore the state of the slave pic
}


/* Enable (unmask) the specified IRQ */
/*
input: irq_num - which irq we want to unmask
output: none
effect: unmasks the given irq number
*/
void enable_irq(uint32_t irq_num)
{
	if(irq_num < 8){								//check if irq is for master
		uint8_t irq_mask = ~(1 << irq_num);			//create mask by inverting the irq number
		master_mask = inb(MASTER_8259_DATA);		//get master mask
		master_mask = master_mask & irq_mask;		//add the mask by using a bitwise AND to make sure we don't overflow
		outb(master_mask, MASTER_8259_DATA);		//send back to master pic
		
	}
	if(irq_num > 7){
		uint8_t irq_mask = ~(1 << (irq_num - 8)); 	//create mask by subtracting the irq position (-8 since its slave) from 0xff
		//printf("IRQ MASK: %d",irq_mask);
		slave_mask = inb(SLAVE_8259_DATA);				//get slave mask
		slave_mask = slave_mask & irq_mask;				//add mask by using a bitwise AND to make sure theres no overflow
		outb(slave_mask, SLAVE_8259_DATA);				//send mask back to slave pic		
	}
}


/* Disable (mask) the specified IRQ */
/*
input: irq_num - irq number to mask 
output: none
effect: disables the given irq number 
*/
void 
disable_irq(uint32_t irq_num)
{
	if(irq_num < 8){								//check if irq is for master
		uint8_t irq_mask = 1 << irq_num;			//if it is, create a mask for the corresponding bit
		master_mask = inb(MASTER_8259_DATA);		//get master mask from pic
		master_mask = master_mask | irq_mask;		//add the mask by using a bitwise OR to make sure we don't overflow
		outb(master_mask, MASTER_8259_DATA); 		//send mask back to master pic
	}
	if(irq_num > 7){								//check if irq is for slave
		uint8_t irq_mask = 1 << (irq_num - 8);		//if it is, create mask for corresponding bit. we have to sub 8 since master is 0-7 bits
		slave_mask = inb(SLAVE_8259_DATA);			//get slave mask from slave pic
		slave_mask = slave_mask | irq_mask;			//add the mask by using a bitwise OR to make sure there is no overflow
		outb(slave_mask, SLAVE_8259_DATA);			//send mask to slave pic
	}
}



/* Send end-of-interrupt signal for the specified IRQ */
/*
input: irq_num - the irq to send an EOI to
output: none
effect: sends an EOI to the given irq number to indicate we are done with an interrupt
*/
void
send_eoi(uint32_t irq_num)
{
	if(irq_num > 7)						//check if its going to master or slave
	{
		outb(EOI|(irq_num - 8),SLAVE_8259_CTRL);	//if slave, we need to send to both master and slave
		outb(EOI|2,MASTER_8259_CTRL);			//send to master as well
	}
	else
		outb(EOI|irq_num,MASTER_8259_CTRL);		//otherwise, we just need to notify the master
}
