#ifndef _PAGING_H
#define _PAGING_H

#include "x86_desc.h"


#define GLOBAL_FLAG 0x100
#define PAGE_SIZE_FLAG 0x80
#define USER_FLAG 0x04
#define RW_FLAG 0x2
#define PRESENT_FLAG 0x1
#define PAGE_4MB 0x80 

#define VIDEO_MEM_OFFSET 184
#define KERNEL_loc 0x00400183 //400000 (4194304) -1 ([0]) + VIDEO_MEM

void paging_init();
int32_t new_process_init(uint32_t process_num);
extern uint32_t process_number;
extern int32_t restore_paging(uint32_t proc_num);
extern int32_t _4kb_video_page();

extern void disable_vidmem(uint32_t PID, unsigned char *video_buffer, uint32_t terminal_no);
extern void enable_vidmem(uint32_t PID, uint32_t terminal_no);
#endif
