#ifndef _IDT_H
#define _IDT_H
#include "types.h"
#include "syscall_linker.h"

/*  Address of handler functions typecasted into integer type */

int idt_handler_addr[256];

void idt_trap(uint32_t x);
void idt_interrupt(uint32_t x);
void idt_system(uint32_t x);

void IDT_init();

//inline void exception_common(); /* This function will be used to implement user-level process termination */

/* 32 Exception Handlers */

extern void divide_by_zero();						//vector: 0x0
extern void debug();								//vector: 0x1
extern void NMI();									//vector: 0x2
extern void breakpoint();							//vector: 0x3
extern void overflow();							//vector: 0x4
extern void bound_range_exceeded();				//vector: 0x5
extern void invalid_opcode();						//vector: 0x6
extern void device_not_available();				//vector: 0x7

extern void double_fault();						//vector: 0x8
extern void segment_overrun();						//vector: 0x9
extern void invalid_TSS();							//vector: 0xA
extern void segment_not_present();					//vector: 0xB
extern void stack_segment_fault();					//vector: 0xC
extern void general_protection_fault();			//vector: 0xD
extern void page_fault();							//vector: 0xE
extern void reserved_exception_1();				//vector: 0xF

extern void floating_point_exception_87();			//vector: 0x10
extern void alignment_check();						//vector: 0x11
extern void machine_check();						//vector: 0x12
extern void floating_point_exception_SIMD();		//vector: 0x13
extern void virtualization_exception();			//vector: 0x14
extern void reserved_exception_2();				//vector: 0x15
extern void reserved_exception_3();				//vector: 0x16
extern void reserved_exception_4();				//vector: 0x17

extern void reserved_exception_5();				//vector: 0x18
extern void reserved_exception_6();				//vector: 0x19
extern void reserved_exception_7();				//vector: 0x1A
extern void reserved_exception_8();				//vector: 0x1B
extern void reserved_exception_9();				//vector: 0x1C
extern void reserved_exception_10();				//vector: 0x1D
extern void security_exception();					//vector: 0x1E
extern void reserved_exception_11();				//vector: 0x1F

extern void idt_ignore();							//placeholder for unused
extern void timer_chip_interrupt();
#endif
