#include "rtc.h"
#include "lib.h"
#include "types.h"
#include "rtc.h"

/*
rtc_init - initialize the RTC and enable it 
input: none
output: none
effect: initializes the rtc
*/
void rtc_init() {
	cli();										//disable ints				
	
	outb(DISABLE_NMI + REGISTER_B, RTC_PORT_1);	//select register b and disable NMI
	char temp = inb(RTC_PORT_2); 				//store prev 0x71 port value
	outb(DISABLE_NMI + REGISTER_B, RTC_PORT_1);	//set index to a again
	outb(temp | 0x40, RTC_PORT_2);				//turn on bit 6 of reg B
	enable_irq(RTC_IRQ);						//enable IRQ 8 on pic	
	sti();										//restore ints
}
/*
rtc_int_enable - set the RTC handler in the IDT
input: none
output: none
effect: sets the assembly linkage for RTC handler for the IDT
*/
void rtc_int_enable(){
	SET_IDT_ENTRY(idt[RTC_INT_VEC], (uint32_t)rtc_wrapper);
}

/*
rtc_open - opens the RTC
input: none
output: none
effect: open the rtc and set the default frequency to 2hz, add to the pcb
*/
int32_t rtc_open(const uint8_t* filename){
	sti();						//allow interrupts
	enable_irq(RTC_IRQ);		//enable rtc 
	rtc_set_frequency(15);		//set it to 2 hz
	return add_pcb_file(curr_pcb, 0, 0); //add it to PCB
}


/*
rtc_close - close the RTC
input: fd
output: 0 on success
effect: close the rtc
*/
int32_t rtc_close(int32_t fd){
	return 0;
}

/*
rtc_read - read the RTC
input: fd, buf - the buffer to use, nbytes - number of bytes to read
output: return 0 on success
effect: wait for next interrupt using a wait flag and while loop
*/
int32_t rtc_read(int32_t fd, void* buf, int32_t nbytes){
	sti();						//enable interrupts
	rtc_wait = 1;				//set flag
	while(rtc_wait);			//wait for interrupt
	
	//printf("RTC_READ WORKS\n");
	return 0;					//return after it has occurred
}

/*
rtc_write - writes the frequency to the RTC
input: fd, buf - the buffer to use, nbytes - number of bytes to write
output: number of bytes able to be written, number of written bytes on success. -1 on fail
effect: change frequency depending on input
*/
int32_t rtc_write(int32_t fd, const void* buf, int32_t nbytes){
	int* buf_temp = (int*)buf;
	
	
	if(nbytes != 4 || buf_temp[0] > 1024)	//check if more than 1024 hertz (our limit) or if they want to write more than four bytes
		return -1;							//return fail

	int rate;	
	switch(buf_temp[0]){					//get the rate from the input hertz
		case 2: rate = 15;					//2 hertz = rate 15
				break;
		case 4: rate = 14;					//4 hertz
				break;
		case 8: rate = 13;					//8
				break;
		case 16: rate = 12;					//etc...
				break;
		case 32: rate = 11;
				break;
		case 64: rate = 10;
				break;
		case 128: rate = 9;
				break;
		case 256: rate = 8;
				break;
		case 512: rate = 7;
				break;
		case 1024: rate = 6;				//this is the max in our kernel, though i have written the rest possible for the rtc
				break;
		case 2048: rate = 5;
				break;
		case 4096: rate = 4;
				break;
		case 8192: rate = 3;				//max of rtc is 8192 hz		
				break;
		default: printf("%d is an invalid RTC Frequency\n", buf_temp[0]);	//else it is an invalid frequency
				return -1;
	}
	//printf("rtc being changed to: %d hertz\n", buf_temp[0]);
	rtc_set_frequency(rate);
	return 4;
}

/*
rtc_set_frequency - sets the frequency to the input rate
input: rate - which speed we want the rtc at, range from 2 to 15
output: none
effect: sets frequency as: freq = 32768 >> (rate -1); 2 is fastest, 15 is slowest (2hz)
*/
void rtc_set_frequency(int rate) {				//sets frequency as: freq = 32768 >> (rate - 1); 2 for fastest, 15 for slowest (2 hz) 
	rate &= 0x0f;								//rate must be between 2 and 15
	cli();										//disable ints
	outb(DISABLE_NMI + REGISTER_A, RTC_PORT_1);	//select register a and disable NMI
	char temp = inb(RTC_PORT_2);				//store prev 0x71 port value

	outb(DISABLE_NMI + REGISTER_A, RTC_PORT_1);	//set index to a again
	outb((temp & 0xf0) | rate, RTC_PORT_2);		//set rate to the lower 4 bits of register a

	sti();										//restore ints
}

/*
rtc_handler - handles the rtc interrupts, sends eoi
input: none
output: none
effect: assembly linked handler for rtc interrupts. activates when an rtc interrupt occurs, clears wait flag
*/
void rtc_handler() {
	//test_interrupts();							//test the interrupts
	outb(REGISTER_C, RTC_PORT_1);				//select register c
	inb(RTC_PORT_2);							//which interrupt happens. discard since we don't care :)
	send_eoi(RTC_IRQ);							//send the EOI
	rtc_wait = 0;
}

