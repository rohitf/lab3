#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include "ext2_fs.h"

int fd = 0;
struct ext2_super_block super;

void super_block()
{
    unsigned int inodes_count = 0, blocks_count = 0, log_block_size = 0;

    pread(fd, &super, sizeof(super), 1024); // DIYU SAYS THIS COULD BE WRONG

    inodes_count = super.s_inodes_count;
    blocks_count = super.s_blocks_count;
    log_block_size = 1024 << super.s_log_block_size; /* calculate block size in bytes */

    printf("SUPERBLOCK,");
    printf(" %d,", blocks_count);
    printf(" %d,", inodes_count);
    printf(" \n");
}

int main(int argc, char *argv[])
{
    fd = open("EXT2_test.img", O_RDONLY);
    super_block();  //don't do recursively like Rohit did
    return 0;
}