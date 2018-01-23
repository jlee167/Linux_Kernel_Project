#ifndef _TERMINAL_H
#define _TERMINAL_H

#include "rtc.h"
#include "keyboard.h"
#include "lib.h"
//#include "filesystem.h"
#include "x86_desc.h"
#include "pcb.h"

#define TERMINAL_BUFF_SIZE 1024
#define VIDEO_BUFF_SIZE 4096
#define DISP_HEIGHT 25
#define DISP_WIDTH 80

//term 0 is the starting one
uint8_t term1_active;
uint8_t term2_active;

uint8_t term1_process;
uint8_t term2_process;

uint8_t curr_terminal; //holds the current terminal number

typedef struct terminal_t
{
	unsigned char buffer[TERMINAL_BUFF_SIZE];
	
	int x; //cursorx
	int y; //cursory

	uint32_t ebp;
	uint32_t esp;
    uint32_t cr3;

    uint8_t curr_process_id;
    uint8_t prev_process_id;
    uint8_t process_count;
    pcb* pcb_ptr[4];
    uint8_t pcb_ptr_flag[4];

} terminal_t;

terminal_t* get_terminals();

int32_t switch_terminals(int32_t target_terminal);

int32_t terminal_open(const uint8_t * filename);
int32_t terminal_close(int32_t fd);

void update_cursor(int x, int y);

int32_t terminal_read(int32_t fd, void* buf, int32_t nbytes);
int32_t terminal_write(int32_t fd, const void* buf, int32_t nbytes);

int32_t scroll_up(unsigned char* history);
void keyboard_helper(unsigned char keystroke);

#endif
