#ifndef _PCB_H
#define _PCB_H

#include "types.h"
#include "lib.h"

typedef int32_t (*open_) (const uint8_t* filename);
typedef int32_t (*read_) (int32_t fd, void* buf, int32_t nbytes);
typedef int32_t (*write_) (int32_t fd, const void* buf, int32_t nbytes);
typedef int32_t (*close_) (int32_t fd);

/*f_ops prototype*/
typedef struct fops_t{
	open_ f_open;
	read_ f_read;
	write_ f_write;
	close_ f_close; 
}fops_t;

/*file desciptor for file array*/
typedef struct file_desc
{
	/*table of file ops*/
	fops_t* f_ops;
	/*for type 2 files, include inode no of file*/
	uint32_t inode_ptr;
	/*every read op will update this*/
	uint32_t file_pos;
	/*1 if file is in use, 0 else*/
	uint32_t file_in_use;
	/*flag for is file array is populated with files*/
	uint32_t no_file;
}file_desc;
/*
typedef struct term_struct
{
	uint32_t terminal_no; 
	uint32_t term_vidmem;
	uint32_t ebp;
	uint32_t esp;
    uint32_t cr3;

}term_struct;*/
    
/*structure of pcb*/
typedef struct pcb
{	
	/*process id of current process*/
	uint32_t pid;
	/*file array structure */
	file_desc file_array[8];
	/*ptr to the parent process's pcb*/
	struct pcb* parent_pcb_ptr;
	/*pid of the parent process*/
	uint32_t parent_pid; 
	
	/*no of files opened in the file array*/
	uint32_t files_opened;
	int pcb_in_use;
	//the terminal the current process is running on
	
	uint32_t halt_status;
	uint32_t cr3;
	uint32_t esp;
	uint32_t ebp;
	uint32_t ps_cr3;
	uint32_t ps_esp;
	uint32_t ps_ebp;

	uint32_t term_pcb_idx;

	uint8_t cmd [32];
	uint8_t args [128];

  	uint8_t arg_size;
}pcb;

/*initializes pcb for a new process*/
extern int32_t init_pcb(pcb* parent_pcb, pcb* pcb_addr);

/*add a new file to the pcb's file array*/
extern int32_t add_pcb_file(pcb* cur_pcb, uint32_t inode_ptr, int32_t type);

/*helper function: adds an rtc file*/
int32_t pcb_add_rtc(uint32_t idx, uint32_t inode_ptr, pcb* cur_pcb);

/*helper function: add a directory file*/
int32_t pcb_add_directory(uint32_t idx, uint32_t inode_ptr, pcb* cur_pcb);

/*helper function: add a type 2 file*/
int32_t pcb_add_normfile(uint32_t idx, uint32_t inode_ptr, pcb* cur_pcb);

/*closes a file in the pcb file array*/
extern int32_t pcb_close_file(uint32_t idx, pcb* cur_pcb);

#endif
