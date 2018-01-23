#include "keyboard.h"
#include "lib.h"
#include "types.h"
#include "x86_desc.h"
#include "idt.h"
#include "terminal.h"

uint8_t get_key[128];
int key_idx = 0;
int caps_on;				//caps lock flag
int shift_on;				//shift flag
int alt_on;					//alt flag

//Key mappings for important keys
enum KEYCODE {
	NULL_KEY = 0,
	L_RELEASED = 0xA6,

	ZERO_PRESSED = 0xB,
	ONE_PRESSED = 0x2,
	
	MINUS_PRESSED = 0x0C,
	EQUAL_PRESSED = 0xD,
	
	COMMA_PRESSED = 0x33,
	POINT_PRESSED = 0x34,
	SLASH_PRESSED = 0x35,
	BACKSLASH_PRESSED = 0x2B,
	
	LEFT_BRACKET_PRESSED = 0x1A,
	RIGHT_BRACKET_PRESSED = 0x1B,
	
	BACKSPACE_PRESSED = 0xE,
	BACKSPACE_RELEASED = 0x8E,
	SPACE_PRESSED = 0x39,
	SPACE_RELEASED = 0xB9,
	ENTER_PRESSED = 0x1C,
	ENTER_RELEASED = 0x9C,

	CAPS_PRESSED = 0x3A,
	CAPS_RELEASED = 0xBA,

	//TAB_PRESSED = 0x0F,
	CTRL_PRESSED = 0x1D,
	CTRL_RELEASED = 0x9D,

	LEFT_SHIFT_PRESSED = 0X2A,
	LEFT_SHIFT_RELEASED = 0XAA,
	RIGHT_SHIFT_PRESSED = 0X36,
	RIGHT_SHIFT_RELEASED = 0XB6,

	ALT_PRESSED = 0x38,
	ALT_RELEASED = 0xB8,
	
	F1_PRESSED = 0xBB,
	F2_PRESSED = 0xBC,
	F3_PRESSED = 0xBD
};

/*
keyboard_init - initializes keyboard on the PIC pin #1
input: none
output: none
effect: enables the keyboard irq on the pic
*/
extern void keyboard_init()
{
	enable_irq(KEYBOARD_IRQ);
}

/*
keyboard_int_enable - maps keyboard handler to interrupt vector #33
input: none
output: none
effect: map keyboard handler to the IDT so we can interpret keypresses
*/
extern void keyboard_int_enable()
{
	SET_IDT_ENTRY(idt[KEYBOARD_INT_VEC], (uint32_t)keyboard_wrapper);
}

/* EDITED from https://github.com/levex/osdev/blob/master/drivers/keyboard.c*/
static char* _qwertyuiop = "qwertyuiop";	//0x10-0x1c
static char* _asdfghjkl = "asdfghjkl";		//1e - 26
static char* _zxcvbnm = "zxcvbnm";			//2c - 32
static char* _num = "1234567890-=";			//2 - d
static char* _special = ";'`"; 				//0x27
static char* _QWERTYUIOP = "QWERTYUIOP";	//caps of q->p
static char* _ASDFGHJKL = "ASDFGHJKL";		//caps of a->l
static char* _ZXCVBNM = "ZXCVBNM";			//caps of z->m
static char* _NUM = "!@#$%^&*()_+";			//shift of 1->=
static char* _SPECIAL = ":\"~";				//shift of special chars

/*
keyboard_to_ascii - Converts scancode to ascii for printing
INPUTS: Scancode from keyboard
OUTPUTS: ASCII of the character
EFFECTS: gets the ascii character for the key that was pressed
*/
uint8_t keyboard_to_ascii(uint8_t key)
{
	//printf("key=0x%x                   \n", key);
	if(key == 0x1C) return '\n';	//enter
	if(key == 0x39) return ' ';
	if(key == 0xE) return '\r';		//backspace
	//if(key == 0x0F) return '\t';    //tab -- useful for testing 
	if(ctrl_on != 0)
	{
		if(key == L_RELEASED) return 0xFF;
	}
	if(shift_on == 0){
		if(key == COMMA_PRESSED) return ',';
		if(key == POINT_PRESSED) return '.';
		if(key == SLASH_PRESSED) return '/';
		if(key == BACKSLASH_PRESSED) return '\\';
		if(key == ZERO_PRESSED) return '0';
		if(key == LEFT_BRACKET_PRESSED) return '[';
		if(key == RIGHT_BRACKET_PRESSED) return ']';
		if(key == MINUS_PRESSED) return '-';
		if(key == EQUAL_PRESSED) return '=';
		if(key >= 0x27 && key <= 0x29){
			return _special[key - 0x27];
		}
	}
	else{
		if(key == COMMA_PRESSED) return '<';
		if(key == POINT_PRESSED) return '>';
		if(key == SLASH_PRESSED) return '?';
		if(key == BACKSLASH_PRESSED) return '|';
		if(key == ZERO_PRESSED) return ')';
		if(key == LEFT_BRACKET_PRESSED) return '{';
		if(key == RIGHT_BRACKET_PRESSED) return '}';
		if(key == MINUS_PRESSED) return '_';
		if(key == EQUAL_PRESSED) return '+';
		if(key >= 0x27 && key <= 0x29){
			return _SPECIAL[key - 0x27];
		}
	}
	if(key >= ONE_PRESSED && key <= EQUAL_PRESSED){
		if(shift_on==0){
			return _num[key - ONE_PRESSED];
		}
		else {
			return _NUM[key - ONE_PRESSED];
		}
	}
	if( ((caps_on==1)&&(shift_on==0)) || ((caps_on==0)&&(shift_on==1)))
	{
		if(key >= 0x10 && key <= 0x1C)
		{
			return _QWERTYUIOP[key - 0x10];
		} 
		else if(key >= 0x1E && key <= 0x26)
		{
			return _ASDFGHJKL[key - 0x1E];
		} 
		else if(key >= 0x2C && key <= 0x32)
		{
			return _ZXCVBNM[key - 0x2C];
		}
		return 0;
	}
	else {
		if(key >= 0x10 && key <= 0x1C)
		{
			return _qwertyuiop[key - 0x10];
		} 
		else if(key >= 0x1E && key <= 0x26)
		{
			return _asdfghjkl[key - 0x1E];
		} 
		else if(key >= 0x2C && key <= 0x32)
		{
			return _zxcvbnm[key - 0x2C];
		}
		return 0;
		
	}
}


/*
keyboard_handler - keyboard interrupt handler 
input: none
output: none
effect: checks keyboard status, sees which key was pressed and sets correct flags or gets the ascii then sends EOI
*/
extern void keyboard_handler()
{	uint8_t c = 0;
	uint8_t keyboard_status;

	/* checks if output buffer is full*/
	keyboard_status = inb(KEYBOARD_STATUS_PORT);

	if(keyboard_status & KEYBOARD_OBF)
	{
		/*get data from keyboard data port*/
		c = inb(KEYBOARD_DATA_PORT);
		/*If caps lock was pressed, check is caps indicator is already on*/
		if(c==CAPS_PRESSED){
			/*if it was not on, turn it on*/
			if(caps_on==0)
			{caps_on=1;}
			/*else turn it off*/
			else
			{caps_on=0;}

			send_eoi(KEYBOARD_IRQ);
			return;
		}	
		if(c==CTRL_PRESSED)
		{
			ctrl_on=1;
			send_eoi(KEYBOARD_IRQ);
			return;
		}
		if(c==CTRL_RELEASED)
		{
			ctrl_on=0;
			send_eoi(KEYBOARD_IRQ);
			return;
		}
		/*if shift was press, turn shift indicator on*/
		if((c==LEFT_SHIFT_PRESSED) || (c==RIGHT_SHIFT_PRESSED))
		{
			shift_on=1;
			send_eoi(KEYBOARD_IRQ);
			return;
		}
		/*if shift was released, turn shift indicator off*/
		if((c==LEFT_SHIFT_RELEASED) || (c==RIGHT_SHIFT_RELEASED))
		{
			shift_on=0;
			send_eoi(KEYBOARD_IRQ);
			return;
		}
		if(c==ALT_PRESSED){
			alt_on = 1;
			send_eoi(KEYBOARD_IRQ);
			return;
		}
		if(c==ALT_RELEASED){
			alt_on = 0;
			send_eoi(KEYBOARD_IRQ);
			return;
		}
		if(c==F1_PRESSED && alt_on){
			//printf("Terminal 1!\n");
			send_eoi(KEYBOARD_IRQ);
			switch_terminals(0);
			return;
		}
		if(c==F2_PRESSED && alt_on){
			//printf("Terminal 2!\n");
			send_eoi(KEYBOARD_IRQ);
			switch_terminals(1);
			return;
		}
		if(c==F3_PRESSED && alt_on){
			//printf("Terminal 3!\n");
			send_eoi(KEYBOARD_IRQ);
			switch_terminals(2);
			return;
		}
		
		/*if it was a character, convert it to ascii and print to screen*/
		c = keyboard_to_ascii(c);
		if( (c != 0xFF) && (((c & 0x80) != 0) || (c == 0)))
		{
			/*no characters, terminate interrupt*/
			//printf("Unknown key pressed.\n" );
			send_eoi(KEYBOARD_IRQ);
			return;
		}
		else
		{
			/*print character to screen*/
			//printf("%c",c);
			keyboard_helper((unsigned char)c);
			send_eoi(KEYBOARD_IRQ);
			return;
		}
	}
	/*terminate interrupt*/
	clear();
	printf("Unknown key pressed.\n" );
	send_eoi(KEYBOARD_IRQ);
}
