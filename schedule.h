#ifndef _SCHEDULE_H
#define _SCHEDULE_H

#include "idt.h"
#include "i8259.h"
#include "idt_asm.h"
#include "x86_desc.h"
#include "terminal.h"

#define PIT_PORT_1 0x43
#define PIT_PORT_2 0x40

typedef struct pcb_linked_t
{
	pcb* pcb;
	struct pcb_linked_t* next;
	struct pcb_linked_t* prev;
} pcb_linked_t;


extern void init_scheduler();
void switch_process(/*int32_t target, int32_t current*/);
extern void init_pit();
extern void pit_int_enable();
extern void pit_handler();

#endif
