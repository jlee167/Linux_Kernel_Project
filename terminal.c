#include "terminal.h"
#include "paging.h"

unsigned char terminal_buffer[TERMINAL_BUFF_SIZE];
unsigned int newline_pressed;

unsigned int index; //index for the buffer
unsigned char prev_disp[DISP_HEIGHT*DISP_WIDTH]; //array of width * height
unsigned char write_disp[DISP_HEIGHT*DISP_WIDTH]; //for term_write
uint8_t char_buffer[7];

terminal_t terminals[3]; //array of terminal structs
static uint8_t video_buffer[3][VIDEO_BUFF_SIZE] __attribute__((aligned (0x4000)));

int terminal_x;
int terminal_y;


terminal_t* get_terminals(){
	return terminals;
}

//may need to get rid of magic numbers later, idk
void update_cursor(int x, int y)
{
	terminal_x = x;
	terminal_y = y;
	set_x(terminal_x);
	set_y(terminal_y);

	uint32_t position = x + y*DISP_WIDTH; //DISP_WIDTH = width
	uint32_t pos_LOW = position & 0xFF; 
	uint32_t pos_HIGH = (position>>8) & 0xFF; //shift byte, then mask
	//outb for low to VGA index register
	outb(0x0F, 0x3D4);
	outb(pos_LOW, 0x3D5);
	//outb for high to VGA index register
	outb(0x0E, 0x3D4); 
	outb(pos_HIGH, 0x3D5);
}

//open up the terminal
int32_t terminal_open(const uint8_t* filename)
{
	int i = 0;
	index = 0; //reset the buffer index

	terminals[0].prev_process_id = '\0';
	terminals[0].curr_process_id = 0;
	terminals[0].process_count = 0;

	term1_active = 0;
	term2_active = 0;
	term1_process = 0;
	term2_process = 0;
	curr_terminal = 0;
	update_cursor(0,0);
	//putc('>');
	//update_cursor(1,0); //reset the cursor
	enable_irq(0x01); //PIC_1, keyboard controller
	//initialize the buffer as empty
	for (i = 0; i < TERMINAL_BUFF_SIZE; i++)
	{
		terminal_buffer[i] = NULL;
	}
	return 0;
}

//term_num is the terminal we will be switching to
int32_t switch_terminals(int32_t target_terminal)
{
	int i;

	//check if no switch required
	if(curr_terminal == target_terminal)
		return -1;

	//not sure if saving ebp is necessary, actually. but keeping for now
	asm volatile("movl %%cr3, %0;"
		:"=a"(terminals[curr_terminal].cr3));
	asm volatile("movl %%ebp, %0;"
		:"=a"(terminals[curr_terminal].ebp));
	asm volatile("movl %%esp, %0;"
		:"=a"(terminals[curr_terminal].esp));
	

	//save the buffer
	memcpy(terminals[curr_terminal].buffer, terminal_buffer,TERMINAL_BUFF_SIZE);
//	for(i = 0; i < TERMINAL_BUFF_SIZE; i++)
//		terminals[curr_terminal].buffer[i] = terminal_buffer[i];

	//update current terminal's cursor position
	terminals[curr_terminal].x = terminal_x;
	terminals[curr_terminal].y = terminal_y;

	//copy video memory to locations in mem we set aside for the terminals
	//haven't defined locs yet, 4kb size
	
	unsigned char* video_pointer = (unsigned char*)0xB8000;
	for (i = 0; i < VIDEO_BUFF_SIZE; i++)
	{
		video_buffer[curr_terminal][i] = video_pointer[i];
		if ( video_pointer[i] == NULL)
			break;
	}
	disable_vidmem(terminals[curr_terminal].curr_process_id, video_buffer[curr_terminal], curr_terminal);
	
	//actually conduct the switch
	clear();
	update_cursor(0, 0);
	curr_terminal = target_terminal;

	//terminals[target_terminal].prev_process_id = 

	if(target_terminal == 1 && term1_active == 0)
	{
		//initialize all of terminal 1's process counters
		terminals[1].prev_process_id = '\0';
		terminals[1].curr_process_id = pid;
		terminals[1].process_count = 0;
		//clear the buffer
		for(i=0; i < TERMINAL_BUFF_SIZE; i++)
		{
			terminal_buffer[i] = '\0';
		}
		term1_active = 1;
		execute((uint8_t*)"shell");
		return 0;
	}
	if(target_terminal == 2 && term2_active == 0)
	{
		//initialize all of terminal 2's process counters
		terminals[2].prev_process_id = '\0';
		terminals[2].curr_process_id = pid;
		terminals[2].process_count = 0;
		//clear the buffer
		for(i=0; i < TERMINAL_BUFF_SIZE; i++)
		{
			terminal_buffer[i] = '\0';
		}
		term2_active = 1;
		execute((uint8_t*)"shell");
		return 0;
	}

	//copy from locations to video memory
	//continue using target_terminal for my own sanity
	for (i = 0; i < VIDEO_BUFF_SIZE; i++)
	{
		video_pointer[i] = video_buffer[curr_terminal][i];
		if ( video_pointer[i] == NULL)
			break;
	}
	enable_vidmem(terminals[curr_terminal].curr_process_id,curr_terminal);
	
	
	//now grab cursor locations
	update_cursor(terminals[target_terminal].x, terminals[target_terminal].y);

	//get the new buffer
	for(i = 0; i < TERMINAL_BUFF_SIZE; i++)
	{
		terminal_buffer[i] = terminals[target_terminal].buffer[i];
	}

	if ( (terminals[target_terminal].esp == 0) || (terminals[target_terminal].ebp == 0) )
	{
	execute((uint8_t *)"shell");
		asm volatile("movl %%ebp, %0;"
			:"=a"(terminals[curr_terminal].ebp));
		asm volatile("movl %%esp, %0;"
			:"=a"(terminals[curr_terminal].esp));
	}
	
	//restore stack pointers

	asm volatile("mov %0, %%esp"
		:
		:"b"(terminals[target_terminal].esp));
	asm volatile("mov %0, %%ebp"
		:
		:"b"(terminals[target_terminal].ebp));
	asm volatile("mov %0, %%cr3"
		:
		:"b"(terminals[target_terminal].cr3));

	return 0;
}

//closes the terminal
int32_t terminal_close(int32_t fd)
{
	disable_irq(0x01); //PIC_!
	return 0;
}

//file descriptor, string?, strlen...
//reading from keyboard into s
int32_t terminal_read(int32_t fd, void* buf, int32_t nbytes)
{
	
	uint8_t * buff = (uint8_t *)buf;
	
	int flag = 0;
	int i;
	int32_t ret;
	//if(fd == 1)
	//{
	//	return -1; //failure
	//}
	newline_pressed = 0;
	//flush the buffer terminal
	for (i = 0; i < TERMINAL_BUFF_SIZE; i++)
	{
		terminal_buffer[i] = NULL;
	}
	while(flag == 0) //loop indefinitely
	{
		
		if(newline_pressed == 1) // wait until enter is pressed
		{
			for(i = 0; (i < nbytes) && (i < TERMINAL_BUFF_SIZE); i++)
			{

				buff[i] = terminal_buffer[i]; //read the buffer
				if(terminal_buffer[i] == '\0') //end of buffer
				{
					buff[i+1]='\n';
					ret = i; //size of buffer, bytes read
					break;
				}
			}
			newline_pressed = 0;
			flag = 1; //we can exit the loop now
		}
		
	}
	return ret;
}

//write from s to terminal
//this function doesn't mess with buffer!
int32_t terminal_write(int32_t fd, const void* buf, int32_t nbytes)
{
	uint8_t * buff = (uint8_t *)buf;
	
	int i;
	//int i, j, k;
	int32_t ret;
	//unsigned char write_disp[nbytes];
	if(fd == 0 || buff == NULL || nbytes < 0)
	{
		return -1; //failure
	}
	cli();
	//flush write_disp, for sanity
	for(i = 0; i < TERMINAL_BUFF_SIZE; i++)
	{
		write_disp[i] = NULL;
	}
	if(terminal_x != 0)
	{
		if(terminal_y == DISP_HEIGHT - 1)
		{
			scroll_up(write_disp);
		}
		//update_cursor(0, terminal_y+1); //start on a new line, if writing while cursor in middle of line
	}
	for(i = 0; i < nbytes; i++)
	{
		//update_cursor(terminal_x, terminal_y);
	
		scroll_up(write_disp); //move up, if needed
		if(buff[i] == '\n') //newline
		{
			putc('\n');
			update_cursor(0, terminal_y + 1); //move down 1
		}
		else if(buff[i] != NULL)
		{
			putc(buff[i]);
			update_cursor(terminal_x+1, terminal_y);
		}
		if((terminal_x == DISP_WIDTH) && (buff[i+1] != '\n'))
		{
			putc('\n');
			update_cursor(0, terminal_y + 1);
		}
		ret++;
	}
	//ret = j + 1;

	//if we are at the bottom as a result of \n from before, we will want to move disp up before updating cursor
	/*if(buff[i-1] == '\n')
	{
		scroll_up(write_disp);
	}*/
	//newline, for our next entry
	//update_cursor(0, terminal_y + 1);
	sti();
	return ret;
}

int32_t scroll_up(unsigned char* history)
{
	int i, j;
	if(terminal_y >= DISP_HEIGHT - 1) //height of screen DISP_HEIGHT, -1
	{
		//save the current display
		for(i = 0; i < DISP_HEIGHT; i++)
		{
			//iterate through width
			for(j = 0; j < DISP_WIDTH; j++)
			{
				//grab current screen -- 0xB8000 is video_mem
				history[i*DISP_WIDTH + j] = *(uint8_t *)(((char *)(0xB8000))+((DISP_WIDTH*i + j)<<1));
			}
		}
		clear();
		update_cursor(0,0); //reset cursor
		//output it all one line higher
		for(i = 0; i < DISP_HEIGHT - 1; i++)
		{
			for(j = 0; j < DISP_WIDTH; j++)
			{
				if(i != 0 && j == 0)
				{
					putc('\n');
				}
				putc(history[(i+1)*DISP_WIDTH + j]);
			}
		}
		update_cursor(0, DISP_HEIGHT-2);
	}
	return j + 1;
}

//takes in keyboard input, for backspace and CTRL+L functionality
void keyboard_helper(unsigned char keystroke)
{
	//int i, j; //two counters
	//int32_t str_eq;

	if(keystroke == 0xFF) //CTRL_L
	{
		clear(); //clear video memory
		update_cursor(0,0);
		//putc('>');
		//update_cursor(1,0);
	}
	/* //TAB originally for testing, no longer needed, I think
	else if(keystroke == '\t') //TAB
	{
		printf("    ");
		update_cursor(terminal_x + 4, terminal_y); //move cursor forward
	}
	*/
	else if(keystroke == '\r') //backspace
	{
		if(index == 0 || (terminal_x == 0 && terminal_y == 0))
		{
			return;
		}
		index--;
		terminal_buffer[index] = NULL;
		if(terminal_x == 0)
		{
			update_cursor(DISP_WIDTH - 1, terminal_y - 1);
		}
		else
		{
			update_cursor(terminal_x-1, terminal_y); //simply move back left
		}
		putc(' ');
		update_cursor(terminal_x, terminal_y); //terminal_x already decremented
	}
	//scrolling
	else if((keystroke == '\n') || (terminal_x > DISP_WIDTH - 1)) //ENTER or reached end of a line
	{
		scroll_up(prev_disp);
		putc('\n');
		update_cursor(0, terminal_y+1);
		//account for hanging character
		if((keystroke != '\n') && (index < TERMINAL_BUFF_SIZE))
		{
			terminal_buffer[index] = keystroke;
			index++;
			putc(keystroke);
			update_cursor(terminal_x+1, terminal_y);
		}
		//newline, where last point may not be end
		if(keystroke == '\n')
		{
			//terminal_buffer[index] = keystroke; //insert the newline char
			newline_pressed = 1;
			index = 0; //reset the buffer index
			//update_cursor(0, terminal_y+1);
			//putc('>');
			update_cursor(terminal_x, terminal_y);
		}
	}
	//anything else, just add to buffer
	else if((index < TERMINAL_BUFF_SIZE) && (keystroke >= 32) && (keystroke <= 126)) // space to tilde
	{
		//band-aid fix for l or L showing up when pressing ctrl+L
		if((keystroke == 'l' || keystroke == 'L') && ctrl_on != 0)
		{
			return;
		}
		terminal_buffer[index] = keystroke; //add to buffer
		index++; //update the index
		putc(keystroke);
		update_cursor(terminal_x+1, terminal_y);
	}
}







