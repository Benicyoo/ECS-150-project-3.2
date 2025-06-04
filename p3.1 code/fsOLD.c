#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#include "disk.h"
#include "fs.h"
struct __attribute__((packed)) superblock{
	char sig[8];
	uint16_t total_blocks;
	uint16_t root;
	uint16_t data_start;
	uint16_t data_block_count;
	uint8_t FAT_blocks;
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
static uint16_t *fat_table = NULL;
static struct root root_dir;
static int mounted = 0;

int fs_mount(const char *diskname)
{
	/* TODO */
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

	//initialize FAT
	//calculate size of table
	uint16_t num_entries = supe.FAT_blocks*BLOCK_SIZE;
	fat_table = malloc(num_entries*sizeof(uint16_t));
	if(fat_table == NULL){
		perror("failed to malloc");
		return -1;
	}
	for(int i = 1; i <= supe.FAT_blocks; i++){
		//read fat table entries from blocks on disk
		//add offset to fat_table to prevent overwriting 
		if(block_read(i, fat_table+(4096*(i-1)))==-1){
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
	/* TODO */
	//free table and reset mounted
	
	if(!mounted){
		perror("not mounted");
		return -1;
	}
	free(fat_table);
	mounted = 0;
	if(block_disk_close()==-1){
		return -1;
	}
	return 0;
}

int fs_info(void)
{
	/* TODO */
	printf("FS Info:\n");
	//print superblock
	printf("total_blk_count=%d\n",supe.total_blocks);
	printf("fat_blk_count=%d\n",supe.FAT_blocks);
	printf("rdir_blk=%d\n",supe.root);
	printf("data_blk=%d\n",supe.data_start);
	printf("data_blk_count=%d\n",supe.data_block_count);
	//free ratio count 0 entries
	int num_entries = (double)supe.FAT_blocks/2*4096;
	int count = 0;
	for(int i = 0; i < num_entries; i++){
		if(fat_table[i]!=0){
			count++;
		}
	}
	printf("fat_free_ratio=%d/%d\n",supe.data_block_count-count, supe.data_block_count);

	//rdir ratio count '\0' filenames
	count = 0;
	for(int i = 0; i < 128; i++){
		if(root_dir.entries[i].filename[0] != '\0'){
			count++;
		}
	}
	printf("rdir_free_ratio=%d/128\n",128-count);
	return 0;
}
