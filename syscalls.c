#include "syscalls.h"
#include "terminal.h"
#include "x86_desc.h"
#include "paging.h"
#include "filesys.h"
#include "x86_desc.h"
#include "syscalls_asm.h"


uint32_t pid; 					//should be initialized as 0, since this is C
uint8_t file_name[FILE_MAX];	//buffer for file name
pcb* curr_pcb;					//the current pcb
uint8_t args[ARG_MAX];			//buffer for arguments
uint32_t arg_size;				//argument size
uint8_t pid_free[PROCESS_MAX]; 			//1 indicates that pid is free


/*
halt - halts the process
input: status - the status to give after halting
output: 0
effect: halts the process and restores the previous paging and registers
*/
int32_t halt (uint8_t status)
{
	int i;
	
	//pass 8-bit status to parent process (expanded to 32-bit)
	pcb* parent_pcb = curr_pcb -> parent_pcb_ptr;
	if(parent_pcb != NULL){
		parent_pcb->halt_status = (uint32_t)status;
	}

	/*free up the pid associated with the process*/
	pid_free[curr_pcb->pid] = 1;
	
	//remove it from the linked list
	//remove_pcb_from_list(curr_pcb->pid);
	//clear the entry from the schedule list
	//clear_pcb(curr_pcb->pid);
	
	//int32_t pcb_ptr; 
	//close file
	for(i = 2; i < 8; i ++)
	{
		if(curr_pcb->file_array[i].file_in_use)
		{
			pcb_close_file(i, curr_pcb);
		}
	}
	
	curr_pcb->pcb_in_use = 0;
	//while(1);

	if(process_number == 1 || (term1_process == 1 && curr_terminal == 1) || (term2_process == 1 && curr_terminal == 2)) //is the first shell
	{
		process_number--;
		//restart the shell, after which pid will be 1 again
		execute((uint8_t*)"shell"); 
		while(1);
	}
	//is not the first shell
	else 
	{
		//restore esp, eip, etc.
		asm volatile ("movl %0, %%esp"
				:
				: "a"(curr_pcb->esp));

		asm volatile ("movl %0, %%ebp"
				:
				: "a"(curr_pcb->ebp));
		
		process_number--;

		restore_paging(curr_pcb->parent_pid);
		tss.ss0 = KERNEL_DS;
		tss.esp0 = KERNEL_START - (curr_pcb->parent_pid*EIGHT_KB)-FOUR;
		
		curr_pcb = curr_pcb->parent_pcb_ptr;
	}

	asm volatile ("movl %0, %%eax"
				:
				: "a"(curr_pcb->halt_status));

	asm volatile ("jmp halt_ret_label");
	return 0;
}

/*
Helper function that parses command and arguments
Inputs: command 
Outputs: none
Side effect: parses command and arguments and fills in their
			 respective arrays
*/
void parse (const uint8_t* command)
{
	int i;
	//for checking spaces, init as 0
	uint8_t offset = 0; 
	uint8_t input_length = 0;
	//0 for cmd, 1 for arg
	uint8_t cmd_arg_switch = 0; 
	uint8_t stripped_spc =0;

	uint8_t cmd_size = 0;
	uint8_t size = 0;

	input_length = strlen((int8_t*)command);

	/*clear file name*/
	for(i = 0; i < FILE_MAX; i++)
	{
		file_name[i] = NULL;
	}

	/*clear args array*/
	for(i = 0; i < ARG_MAX; i++)
	{
		args[i] = '\0';
	}

	//check for leading spaces
	if(command[0] == ' ')
	{	
		for(i = 0; (i < input_length) && (command[i] == ' '); i++)
		{
			offset++;
		}
	}

	for(i = offset; i < input_length; i++)
	{
		if(command[i]=='\0')
			break;

		/*checks if command has been completely parsed*/
		if(cmd_arg_switch == 0 && (command[i] == ' '))
		{
			cmd_arg_switch = 1;
		}

		/*parses out command*/
		if(cmd_arg_switch == 0)
		{
			file_name[cmd_size] = command[i];
			cmd_size++;
		}

		/*stips leading space before arguments and parses them*/
		if(cmd_arg_switch == 1)
		{
			if(stripped_spc == 0)
			{
				while(i < input_length)
				{
					i++;
					if (command[i] == ' '){
						continue;
					}
						break;
				}
				stripped_spc=1;

				if (command[i] != '\0' )
				{
					args[size] = command[i];
					size++;
				}
			}

			else
			{
		   		args[size] = command[i];
				size++;
			}
		}

		if(command[i]=='\0')
			break;
	}
	arg_size = size;
	//printf("parsed:%s\n", curr_pcb->args);
}

/*
Finds a pid that is free
Inputs: none
Outputs: return the pid that is free
Side effect: changes pid array of that no. to 0 (in use)
*/
int32_t get_free_pid()
{
	int i;
	for(i=0; i<PROCESS_MAX; i++)
	{
		if(pid_free[i]==1)
		{
			pid_free[i]=0;
			return i;
		}
	}
	return -1;
}

/*
Copies arguments into the pcb stucture
Inputs: none
Outputs: none
Side effect: pcb will now hold the arugments
*/
void copy_args()
{
	int i;

	/*initializes args[] in pcb*/
	for(i = 0; i < ARG_MAX; i++)
	{
		curr_pcb->args[i] = '\0';
	}

	/*populated args[] in pcb*/
	for(i=0; i< arg_size; i++)
	{
		curr_pcb->args[i] = args[i];
	} 
}

/*
execute - executes a file or command
input: command - what is to be executed
output: -1 on fail, 0 on success
effect: saves registers, sets up paging, executes the file
*/
int32_t execute (const uint8_t* command)
{
	int i;

	/*increment process count*/
	if(process_number >= 6)	
		return -1;

	if (process_number==0)
	{
		for(i=0; i<PROCESS_MAX; i++)
		{
			pid_free[i]=1;
		}
	}
	//Note: remember to update curr_pcb!
	uint32_t prev_cr3;

	//save cr3
	asm volatile ("movl %%CR3, %0"
			:"=b"(prev_cr3)
			:);

	parse(command);
	
	//printf("\nparse\n");
	
	/*Check file is an EXE*/
	int success;
	uint8_t buf[4];
	dentry_t dentry;
	success = read_dentry_by_name(file_name, &dentry);
	if(success==-1)
		return success;
	success = read_data(dentry.finode_type,0,buf,4);
	if (success==-1)
		return success;
	
	/*checks for exe magic numbers*/
	if(!(buf[0]==0x7f && buf[1]==0x45 && buf[2]==0x4c && buf[3]==0x46))	
		return -1;		

	pid = get_free_pid();
	if (pid == -1)
		return pid;
	/*set up new process' paging*/
	new_process_init(pid);
	process_number++;
	if (curr_terminal == 1)
	{
		term1_process++;
	}
	if (curr_terminal == 2)
	{
		term2_process++;
	}
	//pid = process_number;	
	
	/*File loader*/
	program_load(file_name, &dentry);
	
	//printf("program_load\n");
	
	/*setting up TSS for context switch*/
	tss.ss0 = KERNEL_DS;
	tss.esp0 = KERNEL_START - (pid*EIGHT_KB)-FOUR;

	pcb* parent_pcb = curr_pcb; 
	curr_pcb = (pcb*) (KERNEL_START - (pid*EIGHT_KB)-FOUR - 2048);
	curr_pcb->pid = pid;
	
	asm volatile ("movl %%esp, %0"
			: "=b"(curr_pcb->esp)
			:);
	asm volatile ("movl %%ebp, %0"
			: "=b"(curr_pcb->ebp)
			:);
	
	//printf("tss\n");

	//Set up paging
	init_pcb(parent_pcb, curr_pcb);
	curr_pcb->cr3 = prev_cr3;
	curr_pcb->parent_pid = parent_pcb->pid;
	
//	add_pcb_to_list(curr_pcb);	//add it to linked list
	
	/*save arguments in pcb*/
	copy_args();

	extern uint32_t entry_point;
	/*read byte 24-27 of the executable to obtain entry point*/
	read_data(dentry.finode_type, 24, buf, 4);
	
	/*entry point is stored as little endian*/
	entry_point = ((buf[3]<<24) | buf[2]<<16 | buf[1]<<8 | buf[0]);

	/*assembly linkage to set up user stack and switch*/
	context_switch();

	if(curr_pcb->halt_status != -1){
		if((curr_pcb->halt_status>=0)&&(curr_pcb->halt_status<=255))
			return curr_pcb->halt_status;
	}
	return 0;

}

/*
Reads a file
Inputs: file array idx, buf to return read data, no of bytes to read
Outputs: numbers of bytes read; 0 if eof reached, -1 for failure
Side effect: none
*/
int32_t read (int32_t fd, void* buf, int32_t nbytes)
{
	//check if valid
	if(buf == NULL || nbytes < 0 || fd < 0 || fd >= 8 || (*curr_pcb).file_array[fd].file_in_use == 0)
		return -1;
	else
		return ((*curr_pcb).file_array[fd].f_ops)->f_read(fd, buf, nbytes);	
		
}

/*
Writes to a file
Inputs: file array idx, buf that holds data to write, no of bytes to write
Outputs: numbers of bytes written if not all data written; 
		 0 if for success, -1 for failure
Side effect: none
*/
int32_t write (int32_t fd, const void* buf, int32_t nbytes)
{
	if(buf == NULL || nbytes < 0 || fd < 0 || fd >= 8)
		return -1;
	else
		return ((*curr_pcb).file_array[fd].f_ops)->f_write(fd, buf, nbytes);	
}

/*
Opens a file
Inputs: filename of file to be opened
Outputs: file array index of pcb that file is assigned to on success, -1 on failure
Side effect: file can be read or written to
*/
int32_t open (const uint8_t* filename)
{
	int32_t success, type;
	
	/*search if file exists*/
	dentry_t dentry;
	success = read_dentry_by_name(filename, &dentry);
	if (success == -1)
	{
		return success;
	}
	type = dentry.ftype;

	/*depending on the file type call the relevant helper functions*/
	if(type == 0){
		success = rtc_open(filename);
	}
	else if(type==1){
		success = directory_open(filename);
	}
	else if (type==2){
		success = file_open(filename);
	}
	else {return -1;}
	return success;
}

/*
Closes a file
Inputs: file array idx that file has been assigned
Outputs: 0 on success, -1 on failure
Side effect: file can no longer be read from or written to
*/
int32_t close (int32_t fd)
{
	int32_t success;
	success = pcb_close_file(fd, curr_pcb);
	return success;
}

/*
Copies the arguments passed in with the command to pass to a process
Inputs: buffer to copy args into, number of bytes to copy
Outputs: 0 on success, -1 on failure
Side effect: buffer will be filled with the args
*/
int32_t getargs (uint8_t* buf, int32_t nbytes)
{
	
  int32_t i;

  /*error checking*/
  if((buf == NULL) || (nbytes < 0) || arg_size == 0)
    {
      return -1;
    }

   /*copies args into the buffer provided*/ 
   for(i=0; i<nbytes; i++)
   {
    	if(curr_pcb->args[i]=='\0')
    	{
    		buf[i]=curr_pcb->args[i];
    		break;
    	}
    	else
    		buf[i]=curr_pcb->args[i];
    }

	return 0;
}


/*
vidmap - maps video memory to virtual memory that user can use
input: screenstart - the address where we should give the video memory virtual location
output: -1 on fail, 0 on success
effect: creates a 4kb page that maps video memory to user space and to screen start
*/
int32_t vidmap(uint8_t** screen_start)
{
  if(screen_start == NULL || (uint32_t) screen_start < 0x8000000 || (uint32_t) screen_start >= (0x8400000) ) //128 and 128+4 MB locations
    {
      return -1;
    }
	
  *screen_start = (uint8_t *)_4kb_video_page(); //video memory
	return 0;
}

int32_t set_handler (int32_t signum, void* handler)
{
	return 0;
}

int32_t sigreturn (void)
{
	return 0;
}


