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
	/* TODO: Phase 2 */
	(void)filename;
	return 0;
}

int fs_delete(const char *filename)
{
	/* TODO: Phase 2 */
	(void)filename;
	return 0;
}

int fs_ls(void)
{
	/* TODO: Phase 2 */
	return 0;
}

int fs_open(const char *filename)
{
	/* TODO: Phase 3 */
	(void)filename;
	return 0;
}

int fs_close(int fd)
{
	/* TODO: Phase 3 */
	(void)fd;
	return 0;
}

int fs_stat(int fd)
{
	/* TODO: Phase 3 */
	(void)fd;
	return 0;
}

int fs_lseek(int fd, size_t offset)
{
	/* TODO: Phase 3 */
	(void)fd;
	(void)offset;
	return 0;
}

int fs_write(int fd, void *buf, size_t count)
{
	/* TODO: Phase 4 */
	(void)fd;
	(void)buf;
	(void)count;
	return 0;
}

int fs_read(int fd, void *buf, size_t count)
{
	/* TODO: Phase 4 */
	(void)fd;
	(void)buf;
	(void)count;
	return 0;
}
