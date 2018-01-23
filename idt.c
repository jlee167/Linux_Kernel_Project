#include "idt.h"

#include "x86_desc.h"
#include "lib.h"

/********************************************************************

Implementation of Exception Handlers
Only error message printing routine for now.
We need to implement user level process termination in exception_common()

**************************************************************/
/*  Address of handler functions typecasted into integer type */
int idt_handler_addr[256] = {(int)&divide_by_zero, (int)&debug, (int)&NMI, (int)&breakpoint, (int)&overflow, (int)&bound_range_exceeded, (int)&invalid_opcode, (int)&device_not_available,
						(int)&double_fault, (int)&segment_overrun, (int)&invalid_TSS, (int)&segment_not_present, (int)&stack_segment_fault, (int)&general_protection_fault, (int)&page_fault, (int)&reserved_exception_1,
						(int)&floating_point_exception_87, (int)&alignment_check, (int)&machine_check, (int)&floating_point_exception_SIMD, (int)&virtualization_exception, (int)&reserved_exception_3, (int)&reserved_exception_4,
						(int)&reserved_exception_5, (int)&reserved_exception_6, (int)&reserved_exception_7, (int)&reserved_exception_8, (int)&reserved_exception_9, (int)&reserved_exception_10, (int)&security_exception, (int)&reserved_exception_11};


// reference: http://wiki.osdev.org/Interrupt_Descriptor_Table
/* IDT Exception Entries -- TRAP */
void idt_trap(uint32_t x)
{
	idt[x].seg_selector = KERNEL_CS;
	idt[x].reserved4 = 0;	// always 0	
	idt[x].reserved3 = 1;		// type  = 0b1111 for 32 bit trap gate 
	idt[x].reserved2 = 1;
	idt[x].reserved1 = 1;
	idt[x].size = 1;
	idt[x].reserved0 = 0;		// end of type
	idt[x].dpl = 0;				// all exceptions will run in privilege mode
	idt[x].present = 1;			
}

/* IDT Interrupts  -- INTERRUPT */
void idt_interrupt(uint32_t x)
{
	idt[x].seg_selector = KERNEL_CS;
	idt[x].reserved4 = 0;		// always 0
	idt[x].reserved3 = 0;			// type = 0b1110 for 32 bit interrupt gate
	idt[x].reserved2 = 1;
	idt[x].reserved1 = 1;
	idt[x].size = 1;
	idt[x].reserved0 = 0;
	idt[x].dpl = 0;					// interrupts in privilege mode
	idt[x].present = 1;
}

/* IDT System gates -- SYSTEM */
void idt_system(uint32_t x)
{
	idt[x].seg_selector = KERNEL_CS;
	idt[x].reserved4 = 0;			// always 0
	idt[x].reserved3 = 1;				// type = 0b1110 for 32 bit interrupt gate
	idt[x].reserved2 = 1;
	idt[x].reserved1 = 1;
	idt[x].size = 1;
	idt[x].reserved0 = 0;
	idt[x].dpl = 3;		//set to 3, sys calls accessible from userspace
	idt[x].present = 1;
}

/*
IDT_init - initialize IDT
input: none 
output: none
effect: sets the idt entries with the function addresses
*/
void IDT_init()
{	

int i;
	for(i = 0; i < 32; i ++)
	{
		idt_trap(i);
		SET_IDT_ENTRY(idt[i], idt_handler_addr[i]);
	}
	//fill rest with "nothing"
	for(i = 32; i < 256; i ++)
	{
		if (i == 32)
		{
		   idt_interrupt(i);
		   SET_IDT_ENTRY(idt[i], (uint32_t)&timer_chip_interrupt);
		}
		else
		{
		idt_interrupt(i);
		SET_IDT_ENTRY(idt[i], (uint32_t)&idt_ignore);
		}
	}
	idt_system(128);
	SET_IDT_ENTRY(idt[128], (uint32_t)&syscall_linkage); //0x80

}

//Exceptions

inline void exception_common()
{
	cli();
	while(1);
	//halt(-1);
}

void divide_by_zero()					
{
	printf("Exception : Divide by zero");
	exception_common();
}
void debug()
{
	printf("Exception: Debug");
	exception_common();
}
void NMI()
{
	printf("Exception : Non Maskable Interruption");
	exception_common();
}								
void breakpoint()	
{
	printf ("Exception : Breakpoint");
	exception_common();
}						
void overflow()					
{
	printf("Exception : Overflow");
	exception_common();
}			
void bound_range_exceeded()
{
	printf("Exception : Bound range exceeded");
	exception_common();
}
void invalid_opcode()
{
	printf("Exception : Invalid opcode");
	exception_common();
}
void device_not_available()
{
	printf("Exception : Device not available");
	exception_common();
}
void double_fault()
{
	printf("Exception : Double Fault");
	exception_common();
}
void segment_overrun()
{
	printf("Exception : Segmentation Overrun");
	exception_common();
}
void invalid_TSS()
{
	printf("Exception : Invalid TSS");
	exception_common();
}
void segment_not_present()
{
	printf("Exception : Segmentation Not Present");
	exception_common();
}
void stack_segment_fault()
{
	printf("Exception : Stack Segment Fault");
	exception_common();
}
void general_protection_fault()
{
	printf("Exception : General Protection Fault");
	exception_common();
}
void page_fault()
{
	printf("Exception : Page Fault");
	exception_common();
}
void reserved_exception_1()
{
	printf("Exception : Reserved");
	exception_common();
}
void floating_point_exception_87()
{
	printf("Exception : 87 floating point");
	exception_common();
}
void alignment_check()
{
	printf("Exception : Alignment Check");
	exception_common();
}
void machine_check()
{
	printf("Exception : Machine Check");
	exception_common();
}
void floating_point_exception_SIMD()
{
	printf("Exception : SIMD floating point");
	exception_common();
}
void virtualization_exception()
{
	printf("Exception : Virtualization");
	exception_common();
}
void reserved_exception_2()
{
	printf("Exception : Reserve");
	exception_common();
}
void reserved_exception_3()
{
	printf("Exception : Reserve");
	exception_common();
}
void reserved_exception_4()
{
	printf("Exception : Reserve");
	exception_common();
}

void reserved_exception_5()
{
	printf("Exception : Reserve");exception_common();
	exception_common();
}
void reserved_exception_6()
{
	printf("Exception : Reserve");
	exception_common();
}
void reserved_exception_7()
{
	printf("Exception : Reserve");
	exception_common();
}
void reserved_exception_8()
{
	printf("Exception : Reserve");
	exception_common();
}
void reserved_exception_9()
{
	printf("Exception : Reserve");
	exception_common();
}
void reserved_exception_10()
{
	printf("Exception : Reserve");
	exception_common();
}
void security_exception()
{
	printf("Exception : Security");
	exception_common();
}
void reserved_exception_11()
{
	printf("Exception : Reserve");
	exception_common();
}

void timer_chip_interrupt()
{
	printf("Timer Chip Interrupt");
}


void idt_ignore()
{
	printf("Ignored.");
}

