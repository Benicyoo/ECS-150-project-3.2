#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "disk.h"
#include "fs.h"

/* TODO: Phase 1 */
#define BLOCK_SIZE 4096
#define SIGNATURE "ECS150FS"
#define SIGNATURE_LEN 8
#define FAT_EOC 0xFFFF

// TODO: helper function [for fs_write(), fs_read()] that returns the index of the data block corresponding to the file’s offset

// TODO: helper function [for fs_write()] that allocates a new data block and link it at the end of the file’s data block chain

struct __attribute__((packed)) superblock {
    char signature[8];         
    uint16_t total_blocks;  
    uint16_t root;
    uint16_t data_start_index; 
    uint16_t data_block_count; 
    uint8_t fat_blocks; 
    uint8_t padding[4079];
};
struct __attribute__((packed)) root_entry{
	char filename[16];
	uint32_t size;
	uint16_t first_index;
	uint8_t padding[10];
};
struct __attribute__((packed)) root{
	struct root_entry entries[128];
};

struct fd {
    int slot_full; //bool
    struct root_entry *file; 
    size_t offset; //curr byte offset
};

//need fd table in order to support multiple open fds at once
static struct fd fd_table[FS_OPEN_MAX_COUNT];


static struct superblock supe;
static uint16_t *fat = NULL; 
static struct root root_dir;
static int mounted = 0;

int fs_mount(const char *diskname)
{
	/* TODO: Phase 1 */
	if(mounted){
		perror("already open");
		return -1;
	}
	if(block_disk_open(diskname)==-1){
		perror("failed to open");
		return -1;
	}
	//read in super block to memory
	//initialize superblock
	if(block_read(0,&supe)==-1){
		perror("failed to read");
		return -1;
	}
	//CHECK THE SIGNATURE
	if(memcmp(supe.signature, SIGNATURE, SIGNATURE_LEN)==-1){
		perror("SIGNATURE BAD");
		return -1;
	}
	//initialize FAT entries
	//for num data blocks
	// malloc -> free (in unmount)
        fat = malloc(supe.data_block_count*4096 * sizeof(uint16_t));
        if (fat == NULL) {
    		return -1;
        }
	for(int i = 1; i <= supe.fat_blocks; i++){
		//read fat table entries from blocks on disk
		//add offset to fat_table to prevent overwriting 
		if(block_read(i, fat+(4096*(i-1)))==-1){
			perror("failed to read");
			return -1;
		}
	}
	//initialize root directory
	if(block_read(supe.root, &root_dir)==-1){
		perror("failed to read");
		return -1;
	}
	mounted = 1;
	return 0;
}

int fs_umount(void)
{
	/* TODO: Phase 1 */
	//free table and reset mounted
	if(!mounted){
		perror("not mounted");
		return -1;
	}
	free(fat);
	fat = NULL;
	mounted = 0;

	//write FAT & root directory back to disk (Phase 1); use block_write()
	for(int i=0; i < supe.fat_blocks; i++) {
		//block_write document in disk.h
		//fat entries are 2 bytes; block_size/fat_entry_size=#fat entries in blocok
		if(block_write(i+1, fat + i*(BLOCK_SIZE/2 )) == -1) {
			perror("write to FAT failed");
			return -1;
		}
	}
	//write back root directory -> disk
	if (block_write(supe.root, &root_dir) == -1) {
    		perror("failed to write root directory");
    		return -1;
	}


	if(block_disk_close()==-1){
		perror("failed to close");
		return -1;
	}
	return 0;
}

int fs_info(void)
{
	/* TODO: Phase 1 */
	if (!mounted){
		 return -1;
	}
	printf("FS Info:\n");
	
	//print superblock
	printf("total_blk_count=%u\n",supe.total_blocks);
	printf("fat_blk_count=%u\n",supe.fat_blocks);
	printf("rdir_blk=%u\n",supe.root);
	printf("data_blk=%u\n",supe.data_start_index);
	printf("data_blk_count=%u\n",supe.data_block_count);
	//free ratio count 0 entries
	int free_fat = 0;
	for(int i = 0; i < supe.data_block_count; i++){
		if(fat[i]==0){
			free_fat++;
		}
	}
	printf("fat_free_ratio=%d/%d\n",free_fat, supe.data_block_count);

	//rdir ratio count '\0' filenames
	int count = 0;
	for(int i = 0; i < 128; i++){
		if(root_dir.entries[i].filename[0] != '\0'){
			count++;
		}
	}
	printf("rdir_free_ratio=%d/128\n",128-count);
	return 0;

}

int fs_create(const char *filename)
{
	printf("create called");
	/* TODO: Phase 2 */
	/*Return: -1 if no FS is currently mounted, or if @filename is invalid, or if a
 	* file named @filename already exists, or if string @filename is too long, or
 	* if the root directory already contains %FS_FILE_MAX_COUNT files. 0 otherwise. */
	if(!mounted || filename == NULL || strlen(filename) >= FS_FILENAME_LEN) {
		return -1;
	} 
	for(int i=0; i<FS_FILE_MAX_COUNT; i++) {
		//i was profoundly confused by this //lol
		//CHECKS duplicate file name
		if(strcmp(root_dir.entries[i].filename, filename) == 0) {
			return -1;
		}
	}
	
	//init to empty
	for(int i=0; i<FS_FILE_MAX_COUNT; i++) {
		//finds first empty entry
		if(root_dir.entries[i].filename[0] == '\0') {
			//copies name for entry
			strcpy(root_dir.entries[i].filename, filename);
			//init size of entry
			root_dir.entries[i].size = 0;
			//first_index
			root_dir.entries[i].first_index = FAT_EOC;
			return 0;
		}
	}
	//implicit error checking if doesnt return in loop root dir full
	return -1;
}

int fs_delete(const char *filename)
{
	/* TODO: Phase 2 */
	
	/* Return: -1 if no FS is currently mounted, or if @filename is invalid, or if
	 * there is no file named @filename to delete, or if file @filename is currently
	 * open. 0 otherwise.
	 */
	 if(!mounted || filename == NULL) {
	 	return -1;
	 }
	 //loop through files to find filename
	 for(int i=0; i<FS_FILE_MAX_COUNT; i++) {
	 	if(strcmp(root_dir.entries[i].filename, filename) == 0) {
			//found filename in entries
	 		for(int j=0; j<FS_OPEN_MAX_COUNT; j++) {
				//slot_full 1 means fd open ig? and check same file bc mult fds can ->samefile
	 			if(fd_table[j].slot_full && fd_table[j].file == &root_dir.entries[i]) {
	 				return -1; //cant delete open file
	 			}
	 		}
			//get starting index
	 		uint16_t curr_index = root_dir.entries[i].first_index;
	 		uint16_t next_fat_entry;
			while(curr_index != FAT_EOC) {
				//fire emoji
			 	next_fat_entry = fat[curr_index];
			 	fat[curr_index] = 0; 
			 	curr_index = next_fat_entry;
	 		}
	 		
	 	//moved into the if statement
			//double check if we should reassign entire filename length 16
			//root_dir.entries[i].filename[0]='\0';
			//naw just sets first char^^^
			strcpy(root_dir.entries[i].filename, "\0");
			root_dir.entries[i].size = 0;
			//first_index
			root_dir.entries[i].first_index = FAT_EOC;
			
			return 0;
		}
	 }
	 
	//no file in entries matching filename
	return -1;
}

int fs_ls(void)
{
/*
 * fs_ls - List files on file system
 * List information about the files located in the root directory.
 * Return: -1 if no FS is currently mounted. 0 otherwise.
 */
	/* TODO: Phase 2 */
	if(!mounted) {
		return -1;
	}

	printf("FS LS:\n");
	for (int i = 0; i < FS_FILE_MAX_COUNT; i++) {
   		if (root_dir.entries[i].filename[0] != '\0') {
        	printf("file: %s, size: %u, data_blk: %u\n", root_dir.entries[i].filename, root_dir.entries[i].size, root_dir.entries[i].first_index);
    		}
	}
	return 0;
}

int fs_open(const char *filename)
{
	/* TODO: Phase 3 */
	if (!mounted || filename == NULL || strlen(filename) >= FS_FILENAME_LEN) {
        	return -1;
        }

    	for (int i = 0; i < FS_FILE_MAX_COUNT; i++) {
    		//check if file found
        	if (strncmp(root_dir.entries[i].filename, filename, 16) == 0) {
        		//find avail. fd
            		for (int j = 0; j < FS_OPEN_MAX_COUNT; j++) {
                		if (!fd_table[j].slot_full) {
                			//successfully found free fd to use for filename
                    			fd_table[j].slot_full = 1;
                    			fd_table[j].file = &root_dir.entries[i];
                    			fd_table[j].offset = 0;
                    			return j;
                		}
           		}
            return -1; // open file count already at FS_FILE_MAX_COUNT
        }
    }
    return -1; // couldnt open dat hoe
}

int fs_close(int fd)
{
	if (!mounted || fd < 0 || fd >= FS_OPEN_MAX_COUNT || !fd_table[fd].slot_full) {
		return -1;
	}
	
	//reset all the fd vars that were changed when open() succeeded
	fd_table[fd].slot_full = 0;
	fd_table[fd].file = NULL;
	fd_table[fd].offset = 0;
	return 0;
}

int fs_stat(int fd)
{
	/* TODO: Phase 3 */
	// RETURN THE CURR SIZE OF THE FILE fd
	// ! (fd_table[fd].slot_full) for if given fd is empty ?
	if(!mounted || fd<0 || fd>= FS_OPEN_MAX_COUNT || !fd_table[fd].slot_full) {
		return -1;
	}
	return fd_table[fd].file->size;
}

int fs_lseek(int fd, size_t offset)
{
	/* TODO: Phase 3 */
	if(!mounted || fd<0 || fd>= FS_OPEN_MAX_COUNT || !fd_table[fd].slot_full || offset > fd_table[fd].file->size) {
		return -1;
	}
	//set fd.offset to parameter offset
	fd_table[fd].offset = offset;
	return 0;
}

int fs_write(int fd, void *buf, size_t count)
{
	/* TODO: Phase 4 */
/**
 * fs_write - Write to a file
 * @fd: File descriptor
 * @buf: Data buffer to write in the file
 * @count: Number of bytes of data to be written
 */
 	if(!mounted || fd < 0 || fd >= FS_OPEN_MAX_COUNT || !fd_table[fd].slot_full) {
		return -1;
	}
	if(buf == NULL) {
		return -1;
	}
	
	//return the number of bytes actually written
	return count;
}
uint16_t get_block_index(int fd)
{
	uint16_t blocks = 0;
	if(fd_table[fd].offset%BLOCK_SIZE != 0){
		//if not an even block size add 1 bc reading in middle of block
		blocks = fd_table[fd].offset/BLOCK_SIZE + 1;
	}
	else{
		//else just assign integer bc reading at start of block
		blocks = fd_table[fd].offset/BLOCK_SIZE;
	}
	return fd_table[fd].file->first_index+blocks;
}
int fs_read(int fd, void *buf, size_t count)
{
	/* TODO: Phase 4 *///cod
	//he's cooking
	if(!mounted || fd < 0 || fd >= FS_OPEN_MAX_COUNT || !fd_table[fd].slot_full) {
		//not mounted or invalid fd or fd not open
		return -1;
	}//cod
	if(buf == NULL) {
		return -1;
	}
	uint16_t start_index = get_block_index(fd);
	//make "bounce buffer"
	void *bounce = NULL;
	//copy from bounce -> buf (handle mismatches, prof advice)
	//starting reading the block from the offset amount of bytes fuck
		//jump ahead in the bounce dat ass buffer then copy to the buf
		//>:(
		//if reach end of bounce with more to read dec count then read next
		//block
	//buffer offset to avoid overwriting copies
	int buf_offset = 0;
	int bytes_toread = 0;
	int i = 0;
	while(count > 0){
		bytes_toread = BLOCK_SIZE-fd_table[fd].offset;
		//read block into bounce
		block_read(start_index+i,bounce);
		//add to the buffer starting at its offset so dont overwrite
		//add offset to bounce so start reading at correct byte
		//only read till end of block
		memcpy(buf+buf_offset, bounce+fd_table[fd].offset, bytes_toread);
		count=count-fd_table[fd].offset;
	}
	//cod

	//return the num of bytes actually read
	return count;
}//cod
