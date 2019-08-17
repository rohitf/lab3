#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "ext2_fs.h"

int fd = 0, offset = 0;
struct ext2_super_block super;
struct ext2_group_desc grpdes;
const char *FILENAME = "trivial.img";
unsigned int log_block_size = 0;
unsigned int blocks = 0;

#define BASE_OFFSET 1024; /* location of the super-block in the first group */
#define BLOCK_OFFSET(block) (BASE_OFFSET + (block - 1) * block_size);

void group(){
        // GROUP
        // group number (decimal, starting from zero)
        // total number of blocks in this group (decimal)
        // total number of i-nodes in this group (decimal)
        // number of free blocks (decimal)
        // number of free i-nodes (decimal)
        // block number of free block bitmap for this group (decimal)
        // block number of free i-node bitmap for this group (decimal)
        // block number of first block of i-nodes in this group (decimal)

    /*calculate number of block groups on the disk */
    unsigned int num_groups = 1 + (super.s_blocks_count - 1) / super.s_blocks_per_group;

    pread(fd, &grpdes, sizeof(grpdes), offset); // DIYU SAYS THIS COULD BE WRONG
    
    /* calculate size of the group descriptor list in bytes */
    // unsigned int num_group_desc = num_groups * sizeof(struct ext2_group_desc);
    unsigned int grp_num = 0, inode_size = 0, inodes, free_blocks, free_inodes, block_free_number, inode_free_number, first_inode_block;
    blocks = super.s_blocks_count;
    
    pread(fd, &grpdes, sizeof(grpdes), offset); // DIYU SAYS THIS COULD BE WRONG
    
    grp_num = super.s_inodes_count;
    log_block_size = 1024 << super.s_log_block_size; /* calculate block size in bytes */
    inode_size = super.s_inode_size;
    
    printf("GROUP,");
    printf("%d,", 0);
    printf("%d,", blocks);
    printf("%d,", inodes);
    printf("%d,", grpdes.bg_free_blocks_count);
    printf("%d,", grpdes.bg_free_inodes_count);
    printf("%d,", grpdes.bg_block_bitmap);
    printf("%d,", grpdes.bg_inode_bitmap);
    printf("%d\n", grpdes.bg_inode_table);
}

void super_block() {
    unsigned int inodes_count = 0, blocks_count = 0, inode_size = 0;

    pread(fd, &super, sizeof(super), offset); // DIYU SAYS THIS COULD BE WRONG
    offset += sizeof(super);

    inodes_count = super.s_inodes_count;
    blocks_count = super.s_blocks_count;
    log_block_size = 1024 << super.s_log_block_size; /* calculate block size in bytes */
    inode_size = super.s_inode_size;

    unsigned int blocks_per_group = super.s_blocks_per_group;
    unsigned int inodes_per_group = super.s_inodes_per_group;
    unsigned int first_ino = super.s_first_ino;

    printf("SUPERBLOCK,");
    printf("%d,", blocks_count);
    printf("%d,", inodes_count);
    printf("%d,", log_block_size);
    printf("%d,", inode_size);
    printf("%d,", blocks_per_group);
    printf("%d,", inodes_per_group);
    printf("%d", first_ino);
    printf("\n");
}

int is_block_free(int bno, unsigned char * bitmap) 
{
	int index = 0, offset = 0; 
	if (bno == 0)  	
		return 0;

	index = (bno - 1)/sizeof(char);
	offset = (bno - 1)%sizeof(char);
	return ((bitmap[index] & (1 << offset)) == 0);
}

void print_free_blocks()
{
    unsigned char bitmap[blocks/8]; /* allocate memory for the bitmap */
    pread(fd, &bitmap, blocks/8, 1024 + (grpdes.bg_block_bitmap - 1) * 1024); /* read bitmap from disk */

    for (int block = 1; block < blocks; block++)
    {
        printf("Block %d: %d\n", block, is_block_free(block, bitmap));
    }
    
}


void BFree()
{
    int free_block_num = 0;

    printf("BFREE,");
    printf("%d\n", free_block_num);
}

int main(int argc, char *argv[])
{
    fd = open(FILENAME, O_RDONLY);
    offset += 1024;
    super_block(); //don't do recursively like Rohit did
    group();
    print_free_blocks();
    return 0;
}