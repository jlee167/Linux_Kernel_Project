#ifndef _KEYBOARD_H
#define _KEYBOARD_H

#include "idt.h"
#include "i8259.h"
#include "idt_asm.h"
#include "terminal.h"

/*kayboard PIC pin and int vector values*/
#define KEYBOARD_IRQ 1
#define KEYBOARD_INT_VEC 33

/*keyboard ports for reading data*/
#define KEYBOARD_DATA_PORT	0x60
#define KEYBOARD_STATUS_PORT 0x64

/*mask to check if output buffer is full*/
#define KEYBOARD_OBF            0x01

int ctrl_on;

extern void keyboard_init();
extern void keyboard_int_enable();
uint8_t keyboard_to_ascii(uint8_t key);
extern void keyboard_handler();
#endif

