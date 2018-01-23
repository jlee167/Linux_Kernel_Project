/* kernel.c - the C part of the kernel
 * vim:ts=4 noexpandtab
 */

#include "multiboot.h"
#include "x86_desc.h"
#include "lib.h"
#include "i8259.h"
#include "debug.h"
#include "idt.h"
#include "keyboard.h"
#include "rtc.h"
#include "paging.h"
#include "filesys.h"
#include "terminal.h"
#include "pcb.h"
#include "schedule.h"

/* Macros. */
/* Check if the bit BIT in FLAGS is set. */
#define CHECK_FLAG(flags,bit)   ((flags) & (1 << (bit)))

/* Check if MAGIC is valid and print the Multiboot information structure
   pointed by ADDR. */
unsigned int temp_addr;
uint8_t test_buffer[5000];
//int32_t terminal_ret;

void
entry (unsigned long magic, unsigned long addr)
{
	//int i; //counter for test cases

	multiboot_info_t *mbi;

	/* Clear the screen. */
	clear();

	/* Am I booted by a Multiboot-compliant boot loader? */
	if (magic != MULTIBOOT_BOOTLOADER_MAGIC)
	{
		printf ("Invalid magic number: 0x%#x\n", (unsigned) magic);
		return;
	}

	/* Set MBI to the address of the Multiboot information structure. */
	mbi = (multiboot_info_t *) addr;

	/* Print out the flags. */
	printf ("flags = 0x%#x\n", (unsigned) mbi->flags);

	/* Are mem_* valid? */
	if (CHECK_FLAG (mbi->flags, 0))
		printf ("mem_lower = %uKB, mem_upper = %uKB\n",
				(unsigned) mbi->mem_lower, (unsigned) mbi->mem_upper);

	/* Is boot_device valid? */
	if (CHECK_FLAG (mbi->flags, 1))
		printf ("boot_device = 0x%#x\n", (unsigned) mbi->boot_device);

	/* Is the command line passed? */
	if (CHECK_FLAG (mbi->flags, 2))
		printf ("cmdline = %s\n", (char *) mbi->cmdline);

	if (CHECK_FLAG (mbi->flags, 3)) {
		int mod_count = 0;
		int i;
		module_t* mod = (module_t*)mbi->mods_addr;
		while(mod_count < mbi->mods_count) {
			if(mod_count==0){
				temp_addr=mod->mod_start;
			}
			printf("Module %d loaded at address: 0x%#x\n", mod_count, (unsigned int)mod->mod_start);
			printf("Module %d ends at address: 0x%#x\n", mod_count, (unsigned int)mod->mod_end);
			printf("First few bytes of module:\n");
			for(i = 0; i<16; i++) {
				printf("0x%x ", *((char*)(mod->mod_start+i)));
			}
			printf("\n");
			mod_count++;
			mod++;
		}
	}
	/* Bits 4 and 5 are mutually exclusive! */
	if (CHECK_FLAG (mbi->flags, 4) && CHECK_FLAG (mbi->flags, 5))
	{
		printf ("Both bits 4 and 5 are set.\n");
		return;
	}

	/* Is the section header table of ELF valid? */
	if (CHECK_FLAG (mbi->flags, 5))
	{
		elf_section_header_table_t *elf_sec = &(mbi->elf_sec);

		printf ("elf_sec: num = %u, size = 0x%#x,"
				" addr = 0x%#x, shndx = 0x%#x\n",
				(unsigned) elf_sec->num, (unsigned) elf_sec->size,
				(unsigned) elf_sec->addr, (unsigned) elf_sec->shndx);
	}

	/* Are mmap_* valid? */
	if (CHECK_FLAG (mbi->flags, 6))
	{
		memory_map_t *mmap;

		printf ("mmap_addr = 0x%#x, mmap_length = 0x%x\n",
				(unsigned) mbi->mmap_addr, (unsigned) mbi->mmap_length);
		for (mmap = (memory_map_t *) mbi->mmap_addr;
				(unsigned long) mmap < mbi->mmap_addr + mbi->mmap_length;
				mmap = (memory_map_t *) ((unsigned long) mmap
					+ mmap->size + sizeof (mmap->size)))
			printf (" size = 0x%x,     base_addr = 0x%#x%#x\n"
					"     type = 0x%x,  length    = 0x%#x%#x\n",
					(unsigned) mmap->size,
					(unsigned) mmap->base_addr_high,
					(unsigned) mmap->base_addr_low,
					(unsigned) mmap->type,
					(unsigned) mmap->length_high,
					(unsigned) mmap->length_low);
	}

	/* Construct an LDT entry in the GDT */
	{
		seg_desc_t the_ldt_desc;
		the_ldt_desc.granularity    = 0;
		the_ldt_desc.opsize         = 1;
		the_ldt_desc.reserved       = 0;
		the_ldt_desc.avail          = 0;
		the_ldt_desc.present        = 1;
		the_ldt_desc.dpl            = 0x0;
		the_ldt_desc.sys            = 0;
		the_ldt_desc.type           = 0x2;

		SET_LDT_PARAMS(the_ldt_desc, &ldt, ldt_size);
		ldt_desc_ptr = the_ldt_desc;
		lldt(KERNEL_LDT);
	}

	/* Construct a TSS entry in the GDT */
	{
		seg_desc_t the_tss_desc;
		the_tss_desc.granularity    = 0;
		the_tss_desc.opsize         = 0;
		the_tss_desc.reserved       = 0;
		the_tss_desc.avail          = 0;
		the_tss_desc.seg_lim_19_16  = TSS_SIZE & 0x000F0000;
		the_tss_desc.present        = 1;
		the_tss_desc.dpl            = 0x0;
		the_tss_desc.sys            = 0;
		the_tss_desc.type           = 0x9;
		the_tss_desc.seg_lim_15_00  = TSS_SIZE & 0x0000FFFF;

		SET_TSS_PARAMS(the_tss_desc, &tss, tss_size);

		tss_desc_ptr = the_tss_desc;

		tss.ldt_segment_selector = KERNEL_LDT;
		tss.ss0 = KERNEL_DS;
		tss.esp0 = 0x800000;
		ltr(KERNEL_TSS);
	}

	/* Init the PIC */
	
	i8259_init();
	disable_irq(0);			// disable time chip interrupt
	disable_irq(1);			// disable keyboard interrupt
	
	disable_irq(8);	
	/* Init the IDT */
	IDT_init();
	lidt(idt_desc_ptr);
	
	/* Initialize devices, memory, filesystem, enable device interrupts on the
	 * PIC, any other initialization stuff... */
	keyboard_init();

	/*enable rtc*/
	rtc_int_enable();
	rtc_init();
	rtc_set_frequency(15);						//set rate bits to all high which is 2hz

	paging_init();

	/* Enable interrupts */
	keyboard_int_enable();
	
	
	
	/* Do not enable the following until after you have set up your
	 * IDT correctly otherwise QEMU will triple fault and simple close
	 * without showing you any output */
	
	 
	printf("Enabling Interrupts\n");
	sti();

	//int *a = NULL; *a = 1;

	/*test for file system functions*/
	filesys_init(temp_addr);
	//dentry_t dentry;

	
	//This part tests read_dentry_by_name
	//read_dentry_by_name ("verylargetxtwithverylongname.tx", &dentry);
	//printf ("dentry name: %s \n", dentry.fname);
	//printf("dentry inode index: %d \n",dentry.finode_type);
	

	//this part tests read_dentry_by_index 
	//(goes through the whole directory and prints)
	//read_directory();

	//int bytes_read;
	//uint8_t buf[5300];
	//bytes_read = read_data(16,0,buf,5300);
	//printf("%s",buf);
	//printf("%d", bytes_read);

	//clear();
	terminal_open(0);



//FOR TESTING TERMINAL

//only for testing massive
	/*
terminal_read(0, test_buffer, 128);
terminal_write(1, test_buffer, 128);

for(i = 0; i < 4021; i++)
{
	test_buffer[i] = 'a';
}
test_buffer[4019] = 'A';
//test_buffer[4020] = '\n';
terminal_write(1, test_buffer, 4020);

terminal_read(0, test_buffer, 128);
terminal_write(1, test_buffer, 128);

//with newlines
for(i = 0; i < 4021; i++)
{
	test_buffer[i] = 'n';
}
test_buffer[4019] = 'N';
test_buffer[3999] = '\n';
test_buffer[4000] = '\n';
test_buffer[4020] = '\n';
terminal_write(1, test_buffer, 4021);

//for testing read, in conjunction with write - loop indefinitely
while(1)
{
	terminal_read(0, test_buffer, 128);
	terminal_write(1, test_buffer, 128);
}
*/

	/*test for RTC functions */
/*	clear();
	uint8_t* rtc_filename = 0;
	int32_t buf[128];
	int32_t rtc_fd = rtc_open(rtc_filename);
	printf("opened rtc\n");
	int32_t rtc_return = rtc_read(rtc_fd, buf, 1);
	printf("rtc read returns: %d on read\n", rtc_return);
	
	buf[0] = 1024;
	printf("writing %d to the rtc\n", buf[0]);
	rtc_write(rtc_fd, buf, 4);
	printf("rtc write complete\n");
	printf("testing read + correct frequency by printing 0->39 at 1024 hz\n");
	
	int rtc_i;
	for(rtc_i = 0; rtc_i < 40; rtc_i++){
		rtc_read(rtc_fd, buf, 1);
		printf("%d", rtc_i);
	}
	printf("\n");
	buf[0] = 2;
	printf("writing %d to the rtc\n", buf[0]);
	rtc_write(rtc_fd, buf, 4);

	printf("testing read + correct frequency by printing 0->9 at 2 hz\n");
	for(rtc_i = 0; rtc_i < 10; rtc_i++){
		rtc_read(rtc_fd, buf, 1);
		printf("%d", rtc_i);
	}
	printf("\n");
	
	buf[0] = 433;
	printf("writing %d to the rtc, should be invalid\n", buf[0]);
	rtc_write(rtc_fd, buf, 4);
	
	int32_t closed_test = rtc_close(rtc_fd);
	printf("rtc closed returns %d\n", closed_test);
*/

	

/*	
	pcb cur_pcb;
	curr_pcb = NULL;
	init_pcb(NULL, curr_pcb);
*/
	clear();
	
	

	//Test RTC with syscall open/write/read/close 
/*	
	uint32_t buf[32];
	int32_t rtc_fd; 
	rtc_fd = open("rtc");			//OPENS RTC
	printf("rtc's fd = %d\n", rtc_fd);
	
	buf[0] = 1024;
	printf("writing %d to the rtc\n", buf[0]);
	write(rtc_fd, buf, 4);
	printf("rtc write complete\n");
	printf("testing read + correct frequency by printing 0->39 at 1024 hz\n");
	
	int rtc_i;
	for(rtc_i = 0; rtc_i < 40; rtc_i++){
		read(rtc_fd, buf, 1);
		printf("%d", rtc_i);
	}
	printf("\n");
	buf[0] = 2;
	printf("writing %d to the rtc\n", buf[0]);
	write(rtc_fd, buf, 4);

	printf("testing read + correct frequency by printing 0->9 at 2 hz\n");
	for(rtc_i = 0; rtc_i < 10; rtc_i++){
		read(rtc_fd, buf, 1);
		printf("%d", rtc_i);
	}
	printf("\n");
	
	buf[0] = 433;
	printf("writing %d to the rtc, should be invalid\n", buf[0]);
	write(rtc_fd, buf, 4);
	
	int32_t closed_fd = close(rtc_fd);
	printf("closed fd gets: %d\n", closed_fd);
*/
	//Test read with a text file and stdout
/*
	int32_t filefd;
	filefd = open("frame0.txt");
	int32_t buf[198];
	read(filefd, (void*)buf, 188);
	write(1, (void*)buf, 187);
	
	int32_t filefd2;
	filefd2 = open("frame1.txt");
	read(filefd2, (void*)buf, 175);
	write(1, (void*)buf, 174);

	int32_t closed_fd = close(filefd);
	printf("closed fd gets: %d\n", closed_fd);
	closed_fd = close(filefd2);
	printf("closed fd gets: %d\n", closed_fd);
*/

	//init_scheduler();

	/* Execute the first program (`shell') ... */

	execute((uint8_t *)"shell");
	//execute("testprint");
	
	/* Spin (nicely, so we don't chew up cycles) */
	
	
	
	asm volatile(".1: hlt; jmp .1;");
}
