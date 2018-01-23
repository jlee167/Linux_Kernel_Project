#include "pcb.h"
#include "filesys.h"
#include "rtc.h"
#include "terminal.h"

/*fuction prototype for default read function*/
int32_t nofunction1(int32_t fd, void* buf, int32_t nbytes) 
{
	return -1;
}

/*fuction prototype for default write function*/
int32_t nofunction2(int32_t fd, const void* buf, int32_t nbytes)
{
	return -1;
}

/*fuction prototype for default open function*/
int32_t nofunction3(const uint8_t* filename)
{
	return -1;
}

/*fuction prototype for default close function*/
int32_t nofunction4(int32_t fd)
{
	return -1;
}

/*function ops table*/
fops_t rtc_fops = {rtc_open, rtc_read, rtc_write, rtc_close};
fops_t file_fops = {file_open, file_read, file_write, file_close};
fops_t stdin = {terminal_open, terminal_read, nofunction2, terminal_close};
fops_t stdout = {terminal_open, nofunction1, terminal_write, terminal_close};
fops_t directory_fops = {directory_open, directory_read, directory_write, directory_close};
fops_t no_file = {nofunction3, nofunction1, nofunction2, nofunction4};

/*
Initializes pcb
Inputs: ptr to parent pcb, addr to current pcb
Outputs: 0 on completion
Side effect: pcb is initilized and can be used
*/
int32_t init_pcb(pcb* parent_pcb, pcb* pcb_addr)
{

	/* set current pcb to the address given*/
	pcb* cur_pcb;
	cur_pcb = pcb_addr;
	cur_pcb->halt_status = -1;
	//cur_pcb->terminal = curr_terminal;

	/*initialize number of opened files*/
	cur_pcb->files_opened = 2;
	cur_pcb->pcb_in_use = 1;

	/*initialize parent pcb*/
	cur_pcb->parent_pcb_ptr = parent_pcb;

	/*initialize stdin*/
	cur_pcb->file_array[0].f_ops = &stdin;
	cur_pcb->file_array[0].file_in_use = 1;

	/*initialize stdout*/
	cur_pcb->file_array[1].f_ops = &stdout;
	cur_pcb->file_array[1].file_in_use = 1;
	
	/*initializes pcb's file array*/
	int i;
	for(i = 2; i<8; i++){
		cur_pcb->file_array[i].file_in_use = 0;
		cur_pcb->file_array[i].no_file = 0;
		cur_pcb->file_array[i].f_ops = &no_file;
		cur_pcb->file_array[i].inode_ptr = 0;
		cur_pcb->file_array[i].file_pos = 0;
	}
	return 0;
	
}

/*
Adds an entry to the file array
Inputs: pcb idx to add rtc to, NA, ptr to the pcb
Outputs: return the idx of pcb that rtc is added to
Side effect: rtc is opened and can be used.
*/
int32_t add_pcb_file(pcb* cur_pcb, uint32_t inode_ptr, int32_t type)
{
	
	uint32_t idx,i;
	idx = cur_pcb->files_opened;
	cur_pcb->files_opened++;

	/*if file array is full, return -1*/
	if(idx >= 8)
		return -1;

	/*checks which file array idx is empty*/
	for(i=2; i<8 ;i++)
	{
		if(cur_pcb->file_array[i].no_file==0)
		{
			idx = i;
			break;
		}
	}

	/*add in the new file desciptor*/
	int success;
	success = -1;

	/*depending on the type of file desc, calls the 
	  corresponding add function*/
	if (type==0)
		success = pcb_add_rtc(idx, inode_ptr, cur_pcb);
	else if (type==1)
		success = pcb_add_directory(idx, inode_ptr, cur_pcb);
	else if (type==2)
		success = pcb_add_normfile(idx, inode_ptr, cur_pcb);
	else return success;
	
	return success;
}

/*
Adds an rtc to the file array
Inputs: file array idx to add rtc to, NA, ptr to the pcb
Outputs: return the idx of file array that rtc is added to
Side effect: rtc is opened and can be used.
*/
int32_t pcb_add_rtc(uint32_t idx, uint32_t inode_ptr, pcb* cur_pcb)
{
	/*map file ops to rtc ops*/
	cur_pcb->file_array[idx].f_ops = &rtc_fops;

	/*not applicable to rtc*/
	cur_pcb->file_array[idx].inode_ptr = 0;

	/*initialize pcb entry*/
	cur_pcb->file_array[idx].file_pos = 0;
	cur_pcb->file_array[idx].file_in_use = 1;
	cur_pcb->file_array[idx].no_file = 1;

	return idx;

}

/*
Adds a directory to the file array
Inputs: file array idx to add directory to, NA, ptr to the pcb
Outputs: return the idx of file array that directory is added to
Side effect: directory is opened and can be used.
*/
int32_t pcb_add_directory(uint32_t idx, uint32_t inode_ptr, pcb* cur_pcb)
{
	/*map file ops to directory ops*/
	cur_pcb->file_array[idx].f_ops = &directory_fops;

	/*not applicable to directory*/
	cur_pcb->file_array[idx].inode_ptr = 0;

	/*initialize pcb entry*/
	cur_pcb->file_array[idx].file_pos = 0;
	cur_pcb->file_array[idx].file_in_use = 1;
	cur_pcb->file_array[idx].no_file = 1;

	return idx;
}

/*
Adds a file to the file array
Inputs: file array idx to add file to, inode no of file, ptr to the pcb
Outputs: return the idx of file array that file is added to
Side effect: file is opened and can be used.
*/
int32_t pcb_add_normfile(uint32_t idx, uint32_t inode_ptr, pcb* cur_pcb)
{
	/*map file ops to file ops*/
	cur_pcb->file_array[idx].f_ops = &file_fops;

	/*fill in with inode number of file*/
	cur_pcb->file_array[idx].inode_ptr = inode_ptr;

	/*initialize pcb entry*/
	cur_pcb->file_array[idx].file_pos = 0;
	cur_pcb->file_array[idx].file_in_use = 1;
	cur_pcb->file_array[idx].no_file = 1;

	return idx;
}

/*
Closes a file in the file array
Inputs: file array idx of file, ptr to the pcb
Outputs: -1 on failure, 0 if file can be closed.
Side effect: file can no longer be edited until reopened.
*/
int32_t pcb_close_file(uint32_t idx, pcb* cur_pcb)
{
	/*error checking: not out of range and not terminal r/w*/
	if ((idx ==0) | (idx==1) | (idx<0) | (idx>7))
		return -1;
	/*if file has been closed, close fails*/
	else if (cur_pcb->file_array[idx].file_in_use==0)
		return -1;
	/*file is open, close file*/
	else if (cur_pcb->file_array[idx].file_in_use){
		cur_pcb->file_array[idx].file_in_use = 0;
		cur_pcb->files_opened--;
		return 0;
	}
	else return -1;
}
