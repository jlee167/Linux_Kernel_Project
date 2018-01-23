#include "filesys.h"
#include "syscalls.h"
#include "pcb.h"


/*
filesys_init - initializes the filesystem
Input: address of filesystem
Output: none
Effect: sets up the filesystem address
*/
void filesys_init(unsigned int addr)
{
	FILESYS_START_ADDR = (uint32_t) addr;
}


/*
directory_open - opens a directory
Input: the filename to open
Output: -1 on fail, 0 on success 
Effect: reads dentry by name and adds it to pcb
*/
int32_t directory_open(const uint8_t* filename)
{
	int32_t success;
	dentry_t dentry;
	success = read_dentry_by_name(filename, &dentry);

	if(success==-1){
		return success;
	}
	uint32_t inode_ptr = dentry.finode_type;
	success = add_pcb_file(curr_pcb, inode_ptr, 1);
	return success;
}


/*
directory_write - nothing really
Input: fd, buf, nbytes
Output: -1
Effect: None, can't happen
*/

int32_t directory_write(int32_t fd, const void* buf, int32_t nbytes)
{
	return -1;
}



/*
directory_read - reads the blocks of data of the file
Input: file descriptor, buffer for data, bytes to read
Output: 01 on fial, 0 on success
Effect: reads data from blocks and copies them into the buffer
*/
int32_t directory_read(int32_t fd, void* buf, int32_t nbytes)
{
	uint32_t offset;
	int i, success;
	dentry_t current_dentry;

	uint8_t* buffer = (uint8_t*)buf;

	offset = curr_pcb->file_array[fd].file_pos;
	curr_pcb->file_array[fd].file_pos++;
	bootblock_t* bootblock;
	bootblock = (bootblock_t*)FILESYS_START_ADDR;
	
	uint32_t entries_no;

	entries_no = bootblock->dir_no;
	 if(offset >= entries_no)
	 	return 0;
	 else if (offset <0)
	 	return -1;
	 else{
	 	success = read_dentry_by_index(offset, &current_dentry);
	 	for(i=0; i<FNAME_MAX_CHAR; i++)
	 	{
	 		buffer[i]=current_dentry.fname[i];
	 		success = i;
	 	}
	 	return success;
	 }

}


/*
Closes a directory
Inputs: file descriptor
Outputs: -1 on failure and 0 on success
Effects: Sets the fd in the array to not in use
*/
 int32_t directory_close(int32_t fd)
{
	if((fd == 0) | (fd == 1) | (fd >= 8) | (fd <0))
		return -1;
	if (curr_pcb->file_array[fd].file_in_use){
		curr_pcb->file_array[fd].file_in_use = 0;
		return 0;
	}
	else return -1;
}

/*
Find a directory entry by name
Inputs: filename, a dentry_t ptr
Outputs: -1 on failure and 0 on success
Side effect: if file exists, fills in dentry passed in as parameter
			 with the information of the specfic file.
*/
int32_t read_dentry_by_name (const uint8_t* fname, dentry_t* dentry){

	int i, j, char_mismatch;
	bootblock_t* bootblock;
	dentry_t* curr_dentry;
	uint8_t curr_char;

	/*indicates if broke out of loop due to different name*/
	char_mismatch = 0;
	/*addr of boot block*/
	bootblock = (bootblock_t*)FILESYS_START_ADDR;

	uint32_t entries_no, inode_no, data_no;
	entries_no = bootblock->dir_no;
	inode_no = 	bootblock->inode_no;
	data_no =  bootblock->datablk_no;

	//printf("%d",entries_no);

	/*if there are no dir entries, return failure*/
	if(entries_no == 0)
	{
		//printf("No such file exists.\n");
		return -1;
	}
	
	for(i=0; i<entries_no; i++){

		/* ptr arithmetic to next dir entry*/
		curr_dentry =((dentry_t*)bootblock)+i+1;
		
		/*check if the filename matches*/
		for ( j=0; j<FNAME_MAX_CHAR ; j++)
		{
			curr_char = (uint8_t)curr_dentry->fname[j];
			/*if fnames are not the same break inner loop and move to next entry*/
			if (curr_char != fname[j]){
				char_mismatch = 1;
				break;
			}
			/* if fname is the same, break out of outer loop*/
			if(fname[j]=='\0' && curr_char== '\0'){
				break;
			}

		}

		/*if fname is not the same, move on to next entry*/
		if ((char_mismatch == 1) && (i != entries_no-1)){
			char_mismatch = 0;
			continue;
		}
		/*if fname is not the same and this is the last file, file doesn't exist*/
		else if((char_mismatch == 1) && (i == entries_no-1)){
			//printf("No such file exists.\n");
			return -1;
		}
		/*fname is a match, move to next for loop*/
		else{
			break;
		}

	}

	/*fill in the dentry into into the given dentry_t*/
	for ( j=0; j<FNAME_MAX_CHAR ; j++){
		dentry->fname[j]= curr_dentry->fname[j];
	}
	dentry->ftype = curr_dentry->ftype;
	dentry->finode_type = curr_dentry->finode_type;
	return 0;
}

/*
Find a directory entry by index
Inputs: inode index of a file, a dentry_t ptr
Outputs: -1 on failure and 0 on success
Side effect: if file exists, fills in dentry passed in as parameter
			 with the information of the specfic file.
*/
int32_t read_dentry_by_index (uint32_t index, dentry_t* dentry)
{
	uint32_t j;
	dentry_t* curr_dentry;	
	bootblock_t* bootblock;
	
	bootblock = (bootblock_t*)FILESYS_START_ADDR;

	uint32_t entries_no, inode_no, data_no;
	entries_no = bootblock->dir_no;
	inode_no = 	bootblock->inode_no;
	data_no =  bootblock->datablk_no;

	if(index >= entries_no)
	{
		//printf("No such file exists.\n");
		return -1;
	}
	
	curr_dentry = ((dentry_t*)bootblock) + index + 1;
	
	// if a match is found, copy filename,type,inode to second parameter (dentry)
	for ( j=0; j<FNAME_MAX_CHAR ; j++)
	{
		dentry->fname[j]= curr_dentry->fname[j];			
	}
	dentry->ftype = curr_dentry->ftype;
	dentry->finode_type = curr_dentry->finode_type;
	return 0;

}

/*
Reads the data of a specific file
Inputs: inode index, offset in bytes to start reading file from, buffer to 
		fill in with the data read, length in bytes to read 
Outputs: -1 on failure, 0 if end of file is reached, bytes read if end of file is reached.
Side effect: if file exists, fills in buffer with the relevant data
*/
int32_t read_data (uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length)
{
	uint8_t* curr_inode = NULL;
	uint8_t curr_char;
	uint32_t blk_offset, data_offset, datablk_no, flen, data_copied, curr_offset;
	int j,k;

	k=0;
	data_copied=0;

	bootblock_t* bootblock;
	bootblock = (bootblock_t*)FILESYS_START_ADDR;
	curr_inode = (uint8_t*)FILESYS_START_ADDR;

	uint32_t entries_no, inode_no, data_no;
	entries_no = bootblock->dir_no;
	inode_no = 	bootblock->inode_no;
	data_no =  bootblock->datablk_no;

	/*check if inode number is in the valid range. 
	  if it is not, file has been completely read.[*/
	if (length == 0)
		return 0;

	if (inode > inode_no)
		return 0;

	curr_offset = offset;
	/*ptr to the start of the given inode*/
	curr_inode = curr_inode+(inode+1)*BLOCK_SIZE;
	/*data block to start reading from*/
	blk_offset = offset / BLOCK_SIZE;
	/*byte in the data block to start reading from*/
	data_offset = offset % BLOCK_SIZE;
	/* data block # to start reading from*/
	datablk_no = *(int32_t*)(curr_inode + (blk_offset+1)*ENTRY_FIELD_SIZE);
	//printf("%d \n",datablk_no );
	/*file length*/
	flen = *(uint32_t*)curr_inode;
	//printf("%d\n", flen);

	if(curr_offset == flen)
			return 0;
							
	/*check if data block number is valid*/
	if (datablk_no >= data_no)
		return -1;

	/*ptr to the start of the datablk to read from*/
		uint8_t* curr_datablk = NULL;
		curr_datablk = (uint8_t *) FILESYS_START_ADDR;
		curr_datablk = curr_datablk + (inode_no)*BLOCK_SIZE+ (datablk_no+1)*BLOCK_SIZE;
		while(data_copied < length){
					/*copy data from data block into the buf*/
					for(j=data_offset; j<BLOCK_SIZE; j++){
						curr_char = *(uint8_t*)((uint8_t*)curr_datablk+j);
						//printf("%c",curr_char);
						buf[k]=curr_char;
						k++;
						data_copied++;
						curr_offset++;
	
						if (data_copied==length && curr_offset <= flen)
							return data_copied;
						if(curr_offset == flen)
						{
							//printf("End of file reached, bytes copied: %d \n", data_copied);
							return data_copied;
						}				
					}
					/*end of data block is reached, move on to the next data block*/
					data_offset = 0;
					blk_offset++;
					datablk_no = *(int32_t*)(curr_inode + (blk_offset+1)*ENTRY_FIELD_SIZE);
					curr_datablk = (uint8_t*)FILESYS_START_ADDR;
					curr_datablk = curr_datablk + inode_no*BLOCK_SIZE+ (datablk_no+1)*BLOCK_SIZE;
	
					/*check if data block # is valid*/
					if(datablk_no >= data_no){
						return -1;
					}
		}	
	return 0;
}

/*
Opens a file
Inputs: filename
Outputs: -1 on failure, 0 on success
*/
 int32_t file_open(const uint8_t* filename)
{
	int32_t success;
	dentry_t dentry;
	success = read_dentry_by_name(filename, &dentry);

	if(success==-1){
		return success;
	}
	uint32_t inode_ptr = dentry.finode_type;
	success = add_pcb_file(curr_pcb, inode_ptr, 2);
	return success;
}

/*
Closes a file
Inputs: fd
Outputs: -1 on failure, 0 on success
*/
 int32_t file_close(int32_t fd)
{
	 int32_t success;
	 success = pcb_close_file(fd, curr_pcb);
	 return success;
}

/*
Reads the data of a specific file
Inputs: dentry, buffer, bytes to read
Outputs: -1 on failure, 0 if end of file is reached, bytes read if end of file is reached.
Side effect: if file exists, fills in buffer with the relevant data
*/
 int32_t file_read(int32_t fd, void* buf, int32_t nbytes)
{
	uint32_t offset;
	uint32_t inode;
	int32_t success;
	if(curr_pcb->file_array[fd].file_in_use == 0){
		return -1;
	}
	if(curr_pcb->file_array[fd].no_file == 0){
		return -1;
	}
	if(nbytes==0)
		return 0;
	offset = curr_pcb->file_array[fd].file_pos;
	inode = curr_pcb->file_array[fd].inode_ptr;
	success = read_data(inode, offset, (uint8_t*)buf, nbytes);
	curr_pcb->file_array[fd].file_pos = curr_pcb->file_array[fd].file_pos + success;
	return success;
}

/*
Writes to a file
Inputs: fd, buffer, bytes to write
Outputs: -1 always because this is only a read only file system
Effects: None 
*/
 int32_t file_write(int32_t fd, const void* buf, int32_t nbytes)
{
	//printf("Error: This is a read-only filesystem.\n");
	return -1;
}

/*
Copies a program image from file system to 128MB virtual memory
Input: Empty Dentry, Filename
Output: -1 on failure. 0 on success.
Effects: Loads the image into our memory
*/ 
int32_t program_load (const uint8_t* fname, dentry_t* dentry)
{	
	uint32_t i,j;										
	uint32_t *total_inodes_count, *data_block_base, *inode_base;
	uint32_t end_of_file_flag, program_size_byte, data_block_count;
	
	total_inodes_count 	= (uint32_t*)(FILESYS_START_ADDR + 4);													    	//	total number of inode in file system used to calculate data block address
	data_block_base 	= (uint32_t*)(FILESYS_START_ADDR + ((uint32_t)(*total_inodes_count)+1) * SIZE_4KB_IN_BYTE);		//	starting address of data block 
	inode_base 			= (uint32_t*)(FILESYS_START_ADDR + (dentry->finode_type + 1) * SIZE_4MB_IN_KB);					//  program's inode base address
	program_size_byte 	= *inode_base;																					//  program's size in byte


	
	if (program_size_byte % SIZE_4KB_IN_BYTE == 0)								// Calculating number of data blocks to copy
		data_block_count = program_size_byte / SIZE_4KB_IN_BYTE;				// 1 data block contain 4KB (4096 byte)
	else
		data_block_count = (program_size_byte / SIZE_4KB_IN_BYTE) + 1;

	// TEST BLOCK
	/*
	uint32_t *for_test_only = (uint32_t*) FILESYS_START_ADDR;
	printf("TEST: Filesystem Start: 0x%x                                \n", FILESYS_START_ADDR);
	printf("TEST: Total Directory entries: %d                           \n", *for_test_only);
	printf("TEST: Total Inodes addr: 0x%x                               \n", total_inodes_count);
	printf("TEST: Total Inodes: %d                                      \n", *total_inodes_count);
	printf("TEST: Data Block Base ADDR: 0x%x                            \n", data_block_base);
	printf("TEST: Inode_base: 0x%x                                      \n", inode_base);
	printf("TEST: Program Size in Byte: %d                              \n", program_size_byte);
	printf("TEST: Data Block Count: %d                             		\n", data_block_count);
		
	for (i = 0; i < data_block_count; i++)
	{
		uint32_t block_number = inode_base[i+1];
		printf("Data block Number %d: %d \n", i+1, block_number);
	}
	*/
	
	
	// Alligns the file image from filesystem to virtual memory starting in (128MB + 0x48000)

	end_of_file_flag = 0;
	for(i = 0; i < data_block_count; i++)
	{
		uint8_t *block_base, *temp_img_base;
		uint32_t data_block_number;
		
		data_block_number = inode_base[i+1]; 
		block_base = (uint8_t*)((uint32_t)data_block_base + data_block_number * SIZE_4MB_IN_KB);
		//TEST:: printf("TEST: Loop Block Base: 0x%x \n", block_base);
		temp_img_base  = (uint8_t*)(PROGRAM_IMG_BASE + (SIZE_4KB_IN_BYTE * i));
		//TEST:: printf("TEST: Loop Img Base: 0x%x \n", temp_img_base);
		
		for (j = 0; j < SIZE_4KB_IN_BYTE; j++)
		{
			*temp_img_base = block_base[j];
			if( (++end_of_file_flag) >= program_size_byte)
				break;
			temp_img_base = temp_img_base +1;
		}
		// TEST:: printf("TEST: Loop END ADDR: 0x%x \n", temp_img_base);
	}
	// TEST :: printf("EOF: %d \n", end_of_file_flag);
	return 0;
}

