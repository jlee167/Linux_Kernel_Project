#ifndef _SYSCALLS_H
#define _SYSCALLS_H

#include "lib.h"
#include "types.h"
#include "x86_desc.h"
#include "pcb.h"

#define FILE_MAX 32
#define ARG_MAX 128
#define KERNEL_START 0x800000
#define EIGHT_KB 0x2000
#define FOUR 0x4
#define PROCESS_MAX 6

pcb* curr_pcb;
uint32_t entry_point;
uint32_t pid;

/*parses command for execute function*/
void parse (const uint8_t* command); 

/*copies args into pcb args*/
void copy_args();
int32_t get_free_pid();

int32_t halt (uint8_t status);
int32_t execute (const uint8_t* command);
int32_t read (int32_t fd, void* buf, int32_t nbytes);
int32_t write (int32_t fd, const void* buf, int32_t nbytes);
int32_t open (const uint8_t* filename);
int32_t close (int32_t fd);
int32_t getargs (uint8_t* buf, int32_t nbytes);
int32_t vidmap(uint8_t** screen_start);
int32_t set_handler (int32_t signum, void* handler);
int32_t sigreturn (void);

#endif
