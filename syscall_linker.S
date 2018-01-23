.text

.global syscall_linkage

.global halt
.global execute
.global read
.global write
.global open
.global close
.global getargs
.global vidmap
.global set_handler
.global sigreturn

syscall_linkage:

	//save registers
	#cli
	#pushl %eax
	pushl %edx
	pushl %ecx
	pushl %ebx
	pushl %ebp
	pushl %esi
	pushl %edi


	#pushl %es
	#pushl %ds
	#pushw %fs
	#pushw %gs

	//check syscall number
	cmpl $11, %eax //>=11
	jae invalid_syscall
	cmpl $0, %eax //<=0
	jbe invalid_syscall 
	
	addl $-1, %eax //offset for table
	jmp *syscall_table(, %eax, 4)

invalid_syscall:
	movl $-1, %eax //return -1
	
syscall_end:
	#popw %gs
	#popw %fs
	#popw %ds
	#popw %es
	popl %edi
	popl %esi
	popl %ebp
	popl %ebx
	popl %ecx
	popl %edx
	#sti
	#addl $4, %esp
	iret

//NOTE: PLEASE, PLEASE, make sure that you update the registers that are saved HERE!
//adding 1 to EAX is to undo the change for the jumptable offset

syscall_table:
	.long sc_halt, sc_execute, sc_read, sc_write, sc_open, sc_close, sc_getargs, sc_vidmap, sc_set_handler, sc_sigreturn

sc_halt:
	pushl %ebx
	addl $1, %eax
	call halt
	addl $4, %esp
	jmp syscall_end

sc_execute:
	pushl %ebx
	addl $1, %eax
	call execute
	addl $4, %esp
	jmp syscall_end

sc_read:
	pushl %edx
	pushl %ecx
	pushl %ebx
	addl $1, %eax
	call read
	addl $12, %esp
	jmp syscall_end

sc_write:
	pushl %edx
	pushl %ecx
	pushl %ebx
	addl $1, %eax
	call write
	addl $12, %esp
	jmp syscall_end

sc_open:
	pushl %ebx
	addl $1, %eax
	call open
	addl $4, %esp
	jmp syscall_end

sc_close:
	pushl %ebx
	addl $1, %eax
	call close
	addl $4, %esp
	jmp syscall_end

sc_getargs:
	pushl %ecx
	pushl %ebx
	addl $1, %eax
	call getargs
	addl $8, %esp
	jmp syscall_end

sc_vidmap:
	pushl %ebx
	addl $1, %eax
	call vidmap
	addl $4, %esp
	jmp syscall_end

sc_set_handler:
	pushl %ecx
	pushl %ebx
	addl $1, %eax
	call set_handler
	addl $8, %esp
	jmp syscall_end

sc_sigreturn:
	pushl %ebx
	addl $1, %eax
	call sigreturn
	addl $4, %esp
	jmp syscall_end


