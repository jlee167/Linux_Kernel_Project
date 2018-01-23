#include "schedule.h"
#include "lib.h"
#include "types.h"

pcb_linked_t* pcb_linked_list;
pcb_linked_t* pcb_list_storage[6];

/*
init_scheduler - function that starts scheduling for the kernel
input: none
output: none
effect: runs functions that sets up the scheduling

*/
void init_scheduler(){
	pit_int_enable();
	init_pit();
}

/*
add_pcb_to_list - adds a pcb to a circular linked list for scheduling
input: target - the pcb to add
output: none
effect: inserts the target pcb to head of the list, updating the other nodes to keep it circular
*/
void add_pcb_to_list(pcb* target){
	pcb_linked_t* new_node = pcb_list_storage[target->pid];
	new_node->pcb = target;
	new_node->next = pcb_linked_list;
	new_node->prev = pcb_linked_list->prev;
	pcb_linked_list = new_node;
}

/*
remove_pcb_from_list - removes a pcb from the linked list for scheduling
input: pid - the id of the node to remove
output: none
effect: removes the pid from the linked list, update to keep circular
*/
void remove_pcb_from_list(uint32_t pid){
	pcb_linked_t* curr_node = pcb_list_storage[pid];
	if(pcb_linked_list == curr_node)
		pcb_linked_list = curr_node->next;
	pcb_linked_t* temp = curr_node->next;
	temp->prev = curr_node->prev;
	curr_node->prev->next = temp;
}
/*
clear_pcb - clears the pcb entry from the list storage
input: pid - id of the node to clear
output: none
effect: clears out the entry in the list array
*/
void clear_pcb(uint32_t pid){
	pcb_list_storage[pid]->pcb = NULL;
	pcb_list_storage[pid]->next = NULL;
	pcb_list_storage[pid]->prev = NULL;
}

/*
switch_process - switches the process for scheduling 
input: 
output: none
effect: 
*/
void switch_process(/*int32_t target, int32_t current*/){

	if(!(pcb_linked_list->next)){	//check if there is something in the queue
		return;						//return since nothing to switch to
	}
	pcb* curr_process_pcb = pcb_linked_list->pcb;		//get next pcb up for processing
	pcb* prev_process_pcb = curr_process_pcb->parent_pcb_ptr;		//get parent of the next up process, aka the current running one
	
	pcb_linked_list = pcb_linked_list->next;			//rotate the linked list to go to next process in queue
	
	uint32_t esp, ebp;
	asm volatile("movl %%esp, %0" : "=b"(esp));
	asm volatile("movl %%ebp, %0" : "=b"(ebp));
	
	//save the previous process' progress into the esp and ebp entries in its pcb
	prev_process_pcb->esp = esp;
	prev_process_pcb->ebp = ebp;
	
	asm volatile("movl %0, %%esp" : : "a"(curr_process_pcb->esp));
	asm volatile("movl %0, %%ebp" : : "a"(curr_process_pcb->ebp));
	
	//terminal_t* terminals = get_terminals();
	
	tss.ss0 = KERNEL_DS;
	tss.esp0 = KERNEL_START - (curr_process_pcb->pid*EIGHT_KB)-FOUR;
	
}

/*
init_pit - initializes the PIT and sets it to 20ms interrupts
input: none
output: none
effect: enables the PIT irq, sets the control word, and sets a 20ms count for interrupts
*/
void init_pit(){
	enable_irq(0);					//enable irq 0 for the PIT
	uint32_t count = 1193180 / 50;	//send interrupt every 20ms, 1193180 is default count, divide to get a faster rate. We want 20 hz so we divide by 50 
	uint8_t control_word = 0x34;	//create control word
	
	outb(control_word, PIT_PORT_1);	//send the control word to the PIT
	
	outb((uint8_t)(count & 0xf), PIT_PORT_2);	//send the LSB of the count to the PIT
	outb((uint8_t)(count >> 8), PIT_PORT_2);	//send the MSB of the count to the PIT
	
}

/*
pit_int_enable - enables the PIT handler in the IDT
input: none
output: none
effect: sets the IDT entry for the PIT handler
*/
void pit_int_enable(){
	SET_IDT_ENTRY(idt[32], (uint32_t)pit_wrapper);	//maps the pit_wrapper (for handler) to the 32nd idt entry (the entry for pit)
}

/*
pit_handler - function that determines what occurs during PIT interrupt
				in our case it is doing a round robin scheduling switch
input: none
output: none
effect: switches the current process that is being computed, sends eoi
*/
void pit_handler(){
	//printf("p");
	switch_process();
	send_eoi(0);
}
