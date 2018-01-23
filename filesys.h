#ifndef _FILESYS_H
#define _FILESYS_H

#include "lib.h"
#include "types.h"
#include "x86_desc.h"
#include "paging.h"

/*adress of boot block*/
uint32_t FILESYS_START_ADDR;
//void filesys_init(unsigned int addr);
void print_everything();

/*entries are usually 4 bytes in size*/
#define ENTRY_FIELD_SIZE 4

#define BLOCK_SIZE	4096

/*max no of chars for filename*/
#define FNAME_MAX_CHAR 32
#define DIR_ENTRY_SIZE 64

/*offset in the number of bytes for each dir entry*/
#define FILE_TYPE_OFF 		32
#define FILE_INODE_NO_OFF 	36
#define SIZE_4MB_IN_KB 		4096
#define SIZE_4MB_IN_BYTE	(4096 * 1024)
#define SIZE_4KB_IN_BYTE 	4096
#define PDE_INDEX_128MB 	32
#define PROGRAM_IMG_BASE 	0x08048000

/*A file descriptor entry*/
typedef struct bootblock_t{
	uint32_t dir_no;
	uint32_t inode_no;
	uint32_t datablk_no;
} bootblock_t;

extern void filesys_init(unsigned int addr);
extern int32_t program_load (const uint8_t* fname, dentry_t* dentry);

/*directory f_ops*/
extern int32_t directory_open(const uint8_t* filename);
extern int32_t directory_read(int32_t fd, void* buf, int32_t nbytes);
extern int32_t directory_close(int32_t fd);
extern int32_t directory_write(int32_t fd, const void* buf, int32_t nbytes);

/*helper functions to support the f_ops*/
int32_t read_dentry_by_name (const uint8_t* fname, dentry_t* dentry);
int32_t read_dentry_by_index (uint32_t index, dentry_t* dentry);
int32_t read_data (uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length);

/*file f_ops*/
extern int32_t file_open(const uint8_t* filename);
extern int32_t file_close(int32_t fd);
extern int32_t file_read(int32_t fd, void* buf, int32_t nbytes);
extern int32_t file_write(int32_t fd, const void* buf, int32_t nbytes);

#endif
