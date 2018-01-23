#ifndef _RTC_H
#define _RTC_H

#include "idt.h"
#include "i8259.h"
#include "idt_asm.h"
#include "x86_desc.h"

#define RTC_PORT_1 0x70
#define RTC_PORT_2 0x71

#define REGISTER_A 0x0A
#define REGISTER_B 0x0B
#define REGISTER_C 0x0C

#define DISABLE_NMI 0x80
#define RTC_IRQ 8
#define RTC_INT_VEC 40

volatile uint8_t rtc_wait;		//wait flag for rtc read

/*rtc_initialization function*/
extern void rtc_init();
/*sets the rtc's handler in the idt through assembly linkage*/
extern void rtc_int_enable();
/*open the rtc*/
extern int32_t rtc_open(const uint8_t* filename);
/*close the rtc*/
extern int32_t rtc_close(int32_t fd);
/*wait for interrupt*/
extern int32_t rtc_read(int32_t fd, void* buf, int32_t nbytes);
/*write new frequency to rtc*/
extern int32_t rtc_write(int32_t fd, const void* buf, int32_t nbytes);
/*sets the frequency of the rtc*/
extern void rtc_set_frequency(int rate);
/*idt handler for rtc interrupts*/
extern void rtc_handler();

#endif
