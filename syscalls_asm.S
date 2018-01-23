#define ASM     1
#include "syscalls_asm.h"

.globl context_switch
.globl halt_ret_label

#define USER_CS 0x0023
#define USER_DS 0x002B
#define USER_STACK 0x83ffffc

#must update flags to allow interrupt
#change the addr of user stack to 132 - 4

context_switch:
	#movw  USER_DS, %ds  # set DS to point to user mode data segment
	pushl $USER_DS		# push user level SS
	pushl $USER_STACK	# push user level ESP
	pushf 				# push user level EFLAGS
	popl %eax			
	orl $0x200, %eax
	pushl %eax
	pushl $USER_CS		# push user level CS
	pushl entry_point	# push user level EIP
	iret
halt_ret_label:
	leave
	ret


