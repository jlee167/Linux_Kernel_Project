#include "paging.h"
#include "lib.h"

uint32_t process_number;
/* Structures required for paging */
static uint32_t page_directory[7][PAGE_DIRECTORY_SIZE] __attribute__((aligned (0x4000)));
static uint32_t page_table[PAGE_TABLE_SIZE] __attribute__((aligned (0x4000)));
static uint32_t new_page_table[PAGE_TABLE_SIZE] __attribute__((aligned (0x4000)));
static uint32_t video_page_table[3][PAGE_TABLE_SIZE] __attribute__((aligned (0x4000)));


/*
paging_init - initializes paging
input: none
output: none
effect: sets up the paging directories and tables, moves correct values into cr registers 
*/
void paging_init()
{
	process_number = 0;	//set process = 0
	
	uint32_t cr4;
	uint32_t cr0;
	int i; //counter for loops

	/* initialize page directory/ries -- empty */
	for(i=0; i < PAGE_DIRECTORY_SIZE; i++)
	{
		page_directory[0][i] = RW_FLAG;		//set read/write bit
	}

	/* initialize page table(s) -- empty */
	for(i=0; i < PAGE_TABLE_SIZE; i++)
	{
		page_table[i] = i*0x1000 | 0x03;
	}	
	
	//[0] for video memory
	page_directory[0][0] = (uint32_t)page_table | USER_FLAG | RW_FLAG | PRESENT_FLAG; //SU, presents
	page_directory[0][1] = KERNEL_loc; //kernel - we'll set PSE so we can circumvent page table!

	page_table[VIDEO_MEM_OFFSET] = 0x000B8003; //not "messing" with kernel, so R is 0
										//for future reference, other pages should be 0x000B8007


	/* ENABLE PAGING */									

	/* set CR3 - PDBR, page directory base register */
	asm volatile ("mov %0, %%CR3"
				:
				: "a"(page_directory)); 

	/* set the PSE flag (bit 5) of our local copy of CR4 */
	asm volatile ("mov %%CR4, %0"
				: "=b"(cr4));
	cr4 = cr4 | 0x00000010; //0x10
	asm volatile ("mov %0, %%CR4"
				:
				: "a"(cr4));

	/* enable paging by setting CR0 accordingly ( bit 31) */
	asm volatile ("mov %%CR0, %0"
				: "=b"(cr0));
	cr0 = cr0 | 0x80000000;
	asm volatile ("mov %0, %%CR0"
				:
				: "a"(cr0));
}

/*
new_process_init
input: process-num - the process number to initialize
output: 0 for success, -1 for fail
effect: creates and switches to a new page directory for the new process
*/
//0x80 = page size, 0x100 = global flag, 0x1000 = page address 
int32_t new_process_init(uint32_t process_num){
	if(process_num>=6)
		return -1;		//6 is max amount of processes	
	uint32_t * process_page = (uint32_t *)(page_directory[process_num]);		//get address for the new process's place in page directory
	unsigned int i;									//counter
	for(i=0; i < PAGE_TABLE_SIZE; i++){				//iterate over the page
		/* new_*/page_table[i]= (i * 0x1000) | GLOBAL_FLAG /*0x80*/ | RW_FLAG | PRESENT_FLAG ;			//activate supervisor level, global flag, r/w, and mark as present
	}
	/*new_ */page_table[VIDEO_MEM_OFFSET] = 0x000B8003;
	
	process_page[0] = (((uint32_t) /* new_ */page_table) >> 12) << 12 | USER_FLAG | RW_FLAG | PRESENT_FLAG;	//set first entry
	process_page[1] = 0x400000 | GLOBAL_FLAG | PAGE_SIZE_FLAG | RW_FLAG | PRESENT_FLAG;	//set kernel entry
	process_page[0x20] = ((process_num+1)*0x400000 + 0x400000) | PAGE_SIZE_FLAG | USER_FLAG | RW_FLAG | PRESENT_FLAG;
	
	//set the control registers to the page
	asm volatile (
	"movl %0, %%eax;"	//place address to process's page into eax
			:
			: "a"(process_page));
			
	asm volatile (
	//"andl $0xFFFFFFE7, %eax;"
	"movl %eax, %cr3"				//put it into cr3
	);
	return 0;
}

/*
_4kb_video_page - creates a 4kb video page 
input: none
output: virtual address of the mapping
effect: creates a 4kb video page that maps to video memory that is user accessable
*/
int32_t _4kb_video_page(){

	uint32_t * process_page = (uint32_t *)(page_directory[process_number-1]);
	process_page[33] = (uint32_t) new_page_table | USER_FLAG | RW_FLAG | PRESENT_FLAG;
	new_page_table[0] = 0x0B8000 | USER_FLAG | RW_FLAG | PRESENT_FLAG;
	asm volatile ("movl	%cr3,%eax;");
	asm volatile ("movl	%eax,%cr3;");
	return 33*0x400000;
/*	
	uint32_t * process_page = (uint32_t *)(page_directory[process_number]);
	if(!(process_page[31] & 0x1))	//check if user bit is present
		return -1;
	process_page[31] = (0x0B8000) | USER_FLAG | RW_FLAG | PRESENT_FLAG;	//set it to user, rw, and present 
	return (int32_t) &process_page[31];
*/
}

/*
restore_paging - restores the paging for the previous process after one is finished
input: proc_num - the process that is finishing
output: 0
effect: restores the cr3 register to the previous process
*/

void disable_vidmem(uint32_t PID, uint8_t *video_buffer, uint32_t terminal_no)
{
	uint32_t * process_page = (uint32_t *)(page_directory[PID]);
	process_page[33] = (uint32_t)video_page_table[terminal_no] | USER_FLAG | RW_FLAG | PRESENT_FLAG;
	video_page_table[terminal_no][0] = (uint32_t)video_buffer | USER_FLAG | RW_FLAG | PRESENT_FLAG;
	asm volatile ("movl	%cr3,%eax;");
	asm volatile ("movl	%eax,%cr3;");
}
void enable_vidmem(uint32_t PID, uint32_t terminal_no)
{
	uint32_t * process_page = (uint32_t *)(page_directory[PID]);
	process_page[33] = (uint32_t)video_page_table[terminal_no] | USER_FLAG | RW_FLAG | PRESENT_FLAG;
	video_page_table[terminal_no][0] = 0x0B8000 | USER_FLAG | RW_FLAG | PRESENT_FLAG;
	asm volatile ("movl	%cr3,%eax;");
	asm volatile ("movl	%eax,%cr3;");
}


int32_t restore_paging(uint32_t proc_num){
	//printf("proc_num = %d", proc_num);
	asm volatile ("movl %0, %%eax"
			:
			: "a"(&(page_directory[proc_num])));
	asm volatile ("movl %eax, %cr3");
	return 0;
}
