#define ASM     1
#include "idt_asm.h"

.extern keyboard_handler, rtc_handler, pit_handler
.globl keyboard_wrapper, rtc_wrapper, pit_wrapper
.align 4

keyboard_wrapper:
	pushal
	call keyboard_handler
	popal
	iret

rtc_wrapper:
	pushal
	call rtc_handler
	popal
	iret

pit_wrapper:
	pushal
	call pit_handler
	popal
	iret
	