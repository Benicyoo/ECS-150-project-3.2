#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "disk.h"
#include "fs.h"

/* defining the data structures corresponding to the blocks containing the meta-information about the file system (superblock, FAT and root directory) */
// from hw descript: use the integer types defined in stdint.h, such as int8_t, uint8_t, uint16_t, etc
//segfault in gradescope; check bounds

#define BLOCK_SIZE 4096
#define SIGNATURE "ECS150FS"
#define SIGNATURE_LEN 8
#define FAT_EOC 0xFFFF

struct superblock {
    char signature[8];         
    uint16_t total_blocks;  
    uint16_t root_index;
    uint16_t data_start_index; 
    uint16_t data_block_count; 
    uint8_t fat_blocks; 
    // unused/padding
};
static struct superblock sb;
// global mounted -> fixed bug (used in all three fs.c func)
static int mounted = 0;
static uint16_t *fat = NULL; 

int fs_mount(const char *diskname) {
    // open virt. disk w/ block api
    if (block_disk_open(diskname) == -1) {
    	return -1;
    }

    uint8_t block[BLOCK_SIZE];
    if (block_read(0, block) == -1) {
    	return -1;
    }

    //virtual disk properly closed
    if (memcmp(block, SIGNATURE, SIGNATURE_LEN) != 0) {
        block_disk_close();
        return -1;
    }

    // superblock: double check vals/sizes
    memcpy(&sb.signature, block, 8);
    sb.total_blocks = *(uint16_t *)(block+0x08);
    sb.root_index = *(uint16_t *)(block+0x0A);
    sb.data_start_index = *(uint16_t *)(block+0x0C);
    sb.data_block_count = *(uint16_t *)(block+0x0E);
    sb.fat_blocks = *(uint8_t  *)(block+0x10);
    
    // malloc -> free (in unmount)
    fat = malloc(sb.data_block_count * sizeof(uint16_t));
    if (fat == NULL) {
    	return -1;
    }
    
    // load fat from disk?? 
    int entries_read = 0;
    uint8_t block_buf[BLOCK_SIZE];
    for (int i = 0; i < sb.fat_blocks && entries_read < sb.data_block_count; i++) {
        if (block_read(1 + i, block_buf) == -1) {
            //free(fat);
            //fat = NULL; // fat free if err? check
            return -1;
        }

        int entries_to_copy = BLOCK_SIZE / sizeof(uint16_t); //double check size? segfault
        if(sb.data_block_count - entries_read < entries_to_copy) {
            entries_to_copy = sb.data_block_count - entries_read;
        }
        memcpy(fat + entries_read, block_buf, entries_to_copy * sizeof(uint16_t));
        entries_read += entries_to_copy;
    }
    
    mounted = 1;
    return 0;
}


int fs_umount(void) {
    if (!mounted) return -1;

    if (block_disk_close() == -1) {
    	return -1;
    }
    
    if(fat != NULL) {
    	free(fat);
    	//re-init
    	fat = NULL;
    }


    mounted = 0;
    return 0;
}


int fs_info(void) {
    if (!mounted) return -1;

    //from reference prgm
    //printf("");
    printf("FS Info:\ntotal_blk_count=%u\n", sb.total_blocks);
    printf("fat_blk_count=%u\n", sb.fat_blocks);
    printf("rdir_blk=%u\n", sb.root_index);
    printf("data_blk=%u\n", sb.data_start_index);
    printf("data_blk_count=%u\n", sb.data_block_count);
    
    int free_fat = 0;
    for (int i = 1; i < sb.data_block_count; i++) {
    	if (fat[i] == 0) {
    	    free_fat++;
    	}
    }
    printf("fat_free_ratio=%d/%d\n", free_fat, sb.data_block_count);
    
    uint8_t root_block[BLOCK_SIZE];
    block_read(sb.root_index, root_block);

    int free_dir = 0;
    for (int i = 0; i < FS_FILE_MAX_COUNT; i++) {
        if (root_block[i * 32] == '\0') free_dir++;
    }
    printf("rdir_free_ratio=%d/%d\n", free_dir, FS_FILE_MAX_COUNT);
    return 0;
}

