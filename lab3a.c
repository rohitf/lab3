#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include "ext2_fs.h"

int fd = 0, offset = 0;
struct ext2_super_block super;
struct ext2_group_desc grpdes;
const char *FILENAME = "trivial.img";
unsigned int log_block_size = 0;
unsigned int num_blocks = 0;
unsigned int num_inodes = 0;
int logical_byte_offset = 0;

const int BYTE_SIZE = 8;
#define BASE_OFFSET 1024; /* location of the super-block in the first group */
// #define BLOCK_OFFSET(block) (BASE_OFFSET + (block - 1) * block_size);

void group()
{
    // GROUP
    // group number (decimal, starting from zero)
    // total number of num_blocks in this group (decimal)
    // total number of i-nodes in this group (decimal)
    // number of free num_blocks (decimal)
    // number of free i-nodes (decimal)
    // block number of free block bitmap for this group (decimal)
    // block number of free i-node bitmap for this group (decimal)
    // block number of first block of i-nodes in this group (decimal)

    /*calculate number of block groups on the disk */
    unsigned int num_groups = 1 + (super.s_blocks_count - 1) / super.s_blocks_per_group;

    pread(fd, &grpdes, sizeof(grpdes), offset); // DIYU SAYS THIS COULD BE WRONG

    /* calculate size of the group descriptor list in bytes */
    // unsigned int num_group_desc = num_groups * sizeof(struct ext2_group_desc);
    unsigned int grp_num = 0, inode_size = 0, num_inodes, free_blocks, free_inodes, block_free_number, inode_free_number, first_inode_block;
    num_blocks = super.s_blocks_count;

    pread(fd, &grpdes, sizeof(grpdes), offset); // DIYU SAYS THIS COULD BE WRONG
    offset += sizeof(grpdes);

    grp_num = super.s_inodes_count;
    log_block_size = 1024 << super.s_log_block_size; /* calculate block size in bytes */
    inode_size = super.s_inode_size;

    printf("GROUP,");
    printf("%d,", 0);
    printf("%d,", num_blocks);
    printf("%d,", num_inodes);
    printf("%d,", grpdes.bg_free_blocks_count);
    printf("%d,", grpdes.bg_free_inodes_count);
    printf("%d,", grpdes.bg_block_bitmap);
    printf("%d,", grpdes.bg_inode_bitmap);
    printf("%d\n", grpdes.bg_inode_table);
}

void super_block()
{
    unsigned int inode_size = 0;

    pread(fd, &super, sizeof(super), offset); // DIYU SAYS THIS COULD BE WRONG
    offset += sizeof(super);

    num_inodes = super.s_inodes_count;
    num_blocks = super.s_blocks_count;
    log_block_size = 1024 << super.s_log_block_size; /* calculate block size in bytes */
    inode_size = super.s_inode_size;

    unsigned int blocks_per_group = super.s_blocks_per_group;
    unsigned int inodes_per_group = super.s_inodes_per_group;
    unsigned int first_ino = super.s_first_ino;

    printf("SUPERBLOCK,");
    printf("%d,", num_blocks);
    printf("%d,", num_inodes);
    printf("%d,", log_block_size);
    printf("%d,", inode_size);
    printf("%d,", blocks_per_group);
    printf("%d,", inodes_per_group);
    printf("%d", first_ino);
    printf("\n");
}

void print_free_blocks()
{
    unsigned char bitmap[num_blocks / BYTE_SIZE];                                           /* allocate memory for the bitmap */
    pread(fd, &bitmap, num_blocks / BYTE_SIZE, 1024 + (grpdes.bg_block_bitmap - 1) * 1024); /* read bitmap from disk */
    int index = 0, offset = 0;

    for (int block = 1; block <= num_blocks; block++)
    {
        if (block == 0)
            return;

        index = (block - 1) / BYTE_SIZE;
        offset = (block - 1) % BYTE_SIZE;
        bool is_free = ((bitmap[index] & (1 << offset)) == 0);
        if (is_free)
            printf("BFREE,%d\n", block);
    }
}

void print_free_inodes()
{
    unsigned char bitmap[num_inodes / BYTE_SIZE];                                           /* allocate memory for the bitmap */
    pread(fd, &bitmap, num_inodes / BYTE_SIZE, 1024 + (grpdes.bg_inode_bitmap - 1) * 1024); /* read bitmap from disk */
    int index = 0, offset = 0;

    for (int inode = 1; inode <= num_inodes; inode++)
    {
        if (inode == 0)
            return;

        index = (inode - 1) / BYTE_SIZE;
        offset = (inode - 1) % BYTE_SIZE;
        bool is_free = ((bitmap[index] & (1 << offset)) == 0);
        if (is_free)
            printf("IFREE,%d\n", inode);
    }
}

void print_dirents(int block, int parent_inode, int size)
{
    if (block == 0)
        return; // Unallocated block

    unsigned char curr_entry[sizeof(struct ext2_dir_entry)];
    struct ext2_dir_entry *entry;

    lseek(fd, 1024 + (block - 1) * log_block_size, SEEK_SET);
    read(fd, curr_entry, sizeof(struct ext2_dir_entry));
    entry = (struct ext2_dir_entry *)curr_entry;

    if (entry->inode == 0)
        return;

    while (logical_byte_offset < size) // offset is less than the total size of the inode
    {
        // pread(fd, block, log_block_size, (1024 + (address - 1) * log_block_size));

        if (entry->rec_len == 0)
            break;

        char file_name[EXT2_NAME_LEN + 1];
        memcpy(file_name, entry->name, entry->name_len);
        file_name[entry->name_len] = 0; /* append null char to the file name */

        printf("DIRENT,");
        printf("%d,", parent_inode);
        printf("%d,", logical_byte_offset);
        printf("%d,", entry->inode);
        printf("%d,", entry->rec_len);
        printf("%d,", entry->name_len);
        printf("\'%s\'\n", entry->name);

        logical_byte_offset += (entry->rec_len);
        entry = (void *)entry + (entry->rec_len); /* move to the next entry */
    }
}

//with help from https://www.epochconverter.com/programming/c
char *get_gm_time(unsigned long epoch_s)
{
    char *buf = malloc(80);
    time_t epoch = (time_t)epoch_s;
    struct tm ts = *gmtime(&epoch);
    strftime(buf, 80, "%m/%d/%y %H:%M:%S", &ts);
    return buf;
}

int BLOCK_OFFSET(int block_no)
{
    return 1024 + (block_no - 1) * log_block_size;
}

void print_indirect(int block_number, int level, int total_size, int inode_number)
{
    if (level == 0)
    {
        return print_dirents(block_number, inode_number, total_size);
    }

    int num_pointers = log_block_size / sizeof(int);
    int block_pointers[num_pointers];
    pread(fd, &block_pointers, log_block_size, BLOCK_OFFSET(block_number));


    int i, block;
    for (i = 0; i < num_pointers; i++)
    {
        block = block_pointers[i];

        if (block != 0){
            // // Print data
            printf("INDIRECT,");
            printf("%d,", inode_number);
            printf("%d,", level);
            // printf("%d,", logical_block_offset);
            printf("%d,", block_number); // level of indirection
            printf("%d\n", block); // current block pointer
        
            return print_indirect(block, level - 1, total_size, inode_number);
        }
    }
}

void print_inodes()
{
    /* number of inodes per block */
    unsigned int inodes_per_block = log_block_size / sizeof(struct ext2_inode);
    /* size in blocks of the inode table */
    unsigned int itable_blocks = super.s_inodes_per_group / inodes_per_block;

    struct ext2_inode inode_list[num_inodes];
    offset = 1024 + (grpdes.bg_inode_table - 1) * log_block_size;
    pread(fd, &inode_list, sizeof(inode_list), offset); // DIYU SAYS THIS COULD BE WRONG

    int in;
    for (in = 0; in < num_inodes; in++)
    {
        struct ext2_inode curr_in = inode_list[in];

        char *mod_time = get_gm_time(curr_in.i_mtime);
        char *access_time = get_gm_time(curr_in.i_atime);
        char *creat_time = get_gm_time(curr_in.i_ctime);

        if (S_ISDIR(curr_in.i_mode) && curr_in.i_links_count > 0)
        {
            printf("INODE,");
            printf("%d,", in + 1);
            printf("d,");                              // inode #
            printf("%o,", ((curr_in.i_mode) & 0xFFF)); // mode, lower 12 bits
            printf("%d,", curr_in.i_uid);              // Owner
            printf("%d,", curr_in.i_gid);              // Group
            printf("%d,", curr_in.i_links_count);      //how many links to this
            printf("%s,", creat_time);                 //.tm_hour, creat_time.tm_min, creat_time.tm_sec); // Creation time
            printf("%s,", mod_time);                   //.tm_hour, mod_time.tm_min, mod_time.tm_sec); // Creation time
            printf("%s,", access_time);                //.tm_hour, access_time.tm_min, access_time.tm_sec); // Creation time
            printf("%d,", curr_in.i_size);
            printf("%d,", curr_in.i_blocks);

            int i;
            for (i = 0; i < EXT2_N_BLOCKS - 1; i++)
                printf("%d,", curr_in.i_block[i]);

            printf("%d\n", curr_in.i_block[EXT2_N_BLOCKS - 1]);

            int level = 0;
            for (i = 0; i < EXT2_N_BLOCKS; i++)
            {
                if (i >= 12)
                    level++;
                int block_no = curr_in.i_block[i];
                print_indirect(block_no, level, curr_in.i_size, in + 1);
            }
        }
    }
}

int main(int argc, char *argv[])
{
    fd = open(FILENAME, O_RDONLY);
    offset += 1024;
    super_block(); //don't do recursively like Rohit did - hahahaha
    group();
    print_free_blocks();
    print_free_inodes();
    print_inodes();
    return 0;
}