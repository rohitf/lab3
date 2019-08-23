// ID: 904599241, 505458185
// EMAIL: sriramsonti1997@gmail.com, rohitfalor@g.ucla.edu
// NAME: Venkata Sai Sriram Sonti, Rohit Falor

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <math.h>
#include "ext2_fs.h"

int fd = 0, offset = 0, num_pointers = 0, start_offset = 0;
struct ext2_super_block super;
struct ext2_group_desc grpdes;
const char *FILENAME = "trivial.img";
unsigned int log_block_size = 1024;
unsigned int num_blocks = 0;
unsigned int num_inodes = 0;
const int BYTE_SIZE = 8;
const int BASE_OFFSET = 1024; /* location of the super-block in the first group */

// Function declarations
void print_super_block();
void print_group();
void print_free_blocks();
void print_free_inodes();
void print_inodes();

void print_inode_details(char type, struct ext2_inode curr_in, int in);
void print_dirents(int block, int parent_inode);
void print_indirect(int block_number, int level, int total_size, int inode_number, char type);

char *get_gm_time(unsigned long epoch_s);
void throwError(char *message, int code);
int BLOCK_OFFSET(int block_no);

// Helpers
void throwError(char *message, int code)
{
    fprintf(stderr, "%s\n", message);
    exit(code);
}

// with help from https://www.epochconverter.com/programming/c
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
    return BASE_OFFSET + (block_no - 1) * log_block_size;
}

int LEVEL_OFFSET(int level)
{
    return (level <= 0) ? 11 : pow(num_pointers, level - 1) + LEVEL_OFFSET(level - 1);
}

// Print functions
void print_group()
{
    pread(fd, &grpdes, sizeof(grpdes), offset); 
    offset += sizeof(grpdes);

    // GROUP
    // group number (decimal, starting from zero)
    // total number of num_blocks in this group (decimal)
    // total number of i-nodes in this group (decimal)
    // number of free num_blocks (decimal)
    // number of free i-nodes (decimal)
    // block number of free block bitmap for this group (decimal)
    // block number of free i-node bitmap for this group (decimal)
    // block number of first block of i-nodes in this group (decimal)

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

void print_super_block()
{
    unsigned int inode_size = 0;

    pread(fd, &super, sizeof(super), offset); 
    offset += sizeof(super);

    num_inodes = super.s_inodes_count;
    num_blocks = super.s_blocks_count;

    // Set useful globals
    log_block_size = 1024 << super.s_log_block_size; /* calculate block size in bytes */
    num_pointers = log_block_size / sizeof(int);

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
    unsigned char bitmap[num_blocks / BYTE_SIZE + 1];                                 /* allocate memory for the bitmap */
    pread(fd, &bitmap, num_blocks / BYTE_SIZE, BLOCK_OFFSET(grpdes.bg_block_bitmap)); /* read bitmap from disk */

    int index = 0, block;
    for (block = 1; (unsigned int)block <= num_blocks; block++)
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
    unsigned char bitmap[num_inodes / BYTE_SIZE + 1];                                 /* allocate memory for the bitmap */
    pread(fd, &bitmap, num_inodes / BYTE_SIZE, BLOCK_OFFSET(grpdes.bg_inode_bitmap)); /* read bitmap from disk */

    int index = 0, inode_no;
    for (inode_no = 1; (unsigned int)inode_no <= num_inodes; inode_no++)
    {
        if (inode_no == 0)
            return;

        index = (inode_no - 1) / BYTE_SIZE;
        offset = (inode_no - 1) % BYTE_SIZE;
        bool is_free = ((bitmap[index] & (1 << offset)) == 0);
        if (is_free)
            printf("IFREE,%d\n", inode_no);
    }
}

// inspired by http://cs.smith.edu/~nhowe/Teaching/csc262/oldlabs/ext2.html
// goes to data block and prints directory entries within that 1 block
void print_dirents(int block, int inode_number)
{
    unsigned char entries[log_block_size];
    struct ext2_dir_entry *curr_entry;

    if (block == 0)
        return; // Unallocated block

    pread(fd, entries, log_block_size, BLOCK_OFFSET(block));

    curr_entry = (struct ext2_dir_entry *)entries;
    int entry_offset = 0;

    while ((unsigned int)entry_offset < log_block_size) //size is less than the total size of the inode
    {
        if (curr_entry->inode == 0)
            break;

        char file_name[EXT2_NAME_LEN + 1];
        memcpy(file_name, curr_entry->name, curr_entry->name_len);
        file_name[curr_entry->name_len] = 0; /* append null char to the file name */

        printf("DIRENT,");
        printf("%d,", inode_number);
        printf("%d,", entry_offset);
        printf("%d,", curr_entry->inode);
        printf("%d,", curr_entry->rec_len);
        printf("%d,", curr_entry->name_len);
        printf("\'%s\'\n", curr_entry->name);
        entry_offset += curr_entry->rec_len;
        curr_entry = (void *)curr_entry + curr_entry->rec_len; /* move to the next curr_entry */
    }
}

void print_indirect(int block_number, int level, int total_size, int inode_number, char type)
{

    if (level == 0)
    {
        if (type == 'd')
            return print_dirents(block_number, inode_number);
    }
    else
    {
        int block_pointers[num_pointers];
        pread(fd, &block_pointers, sizeof(block_pointers), BLOCK_OFFSET(block_number));

        int i, block;
        for (i = 0; i < num_pointers; i++)
        {

            block = block_pointers[i];

            if (block != 0)
            {
                //printf("IN BLOCK NOT EQUALS 0: %d\n", block);
                printf("INDIRECT,");
                printf("%d,", inode_number);
                printf("%d,", level); // level of indirection
                printf("%d,", i + start_offset);
                printf("%d,", block_number); 
                printf("%d\n", block);       // current block pointer

                print_indirect(block, level - 1, total_size, inode_number, type);
            }
        }
    }
}

void print_inode_details(char type, struct ext2_inode curr_in, int in)
{
    char *mod_time = get_gm_time(curr_in.i_mtime);
    char *access_time = get_gm_time(curr_in.i_atime);
    char *creat_time = get_gm_time(curr_in.i_ctime);

    printf("INODE,");
    printf("%d,", in + 1);
    printf("%c,", type);                       // inode #
    printf("%o,", ((curr_in.i_mode) & 0xFFF)); // mode, lower 12 bits
    printf("%d,", curr_in.i_uid);              // owner
    printf("%d,", curr_in.i_gid);              // group
    printf("%d,", curr_in.i_links_count);      // how many links to this
    printf("%s,", creat_time);                 //.tm_hour, creat_time.tm_min, creat_time.tm_sec); // Creation time
    printf("%s,", mod_time);                   //.tm_hour, mod_time.tm_min, mod_time.tm_sec); // Creation time
    printf("%s,", access_time);                //.tm_hour, access_time.tm_min, access_time.tm_sec); // Creation time
    printf("%d,", curr_in.i_size);
    printf("%d,", curr_in.i_blocks);

    int i;
    for (i = 0; i < EXT2_N_BLOCKS - 1; i++)
        printf("%d,", curr_in.i_block[i]);

    printf("%d\n", curr_in.i_block[EXT2_N_BLOCKS - 1]);
}

void print_inodes()
{
    struct ext2_inode inode_list[num_inodes];
    offset = BLOCK_OFFSET(grpdes.bg_inode_table);
    pread(fd, &inode_list, sizeof(inode_list), offset); 

    int inode_no;
    for (inode_no = 0; (unsigned int)inode_no < num_inodes; inode_no++)
    {
        struct ext2_inode curr_in = inode_list[inode_no];
        int level = 0;

        if (S_ISREG(curr_in.i_mode) && curr_in.i_links_count > 0) // if reg file
        {
            print_inode_details('f', curr_in, inode_no);
            int ifl;
            for (ifl = 0; ifl < EXT2_N_BLOCKS; ifl++)
            {
                if (ifl >= 12)
                {
                    level++;
                    start_offset = LEVEL_OFFSET(level);
                }
                int block_no = curr_in.i_block[ifl];
                print_indirect(block_no, level, curr_in.i_size, inode_no + 1, 'f');
            }
        }
        else if (S_ISDIR(curr_in.i_mode) && curr_in.i_links_count > 0) // if directory
        {
            print_inode_details('d', curr_in, inode_no);

            int i;
            for (i = 0; i < EXT2_N_BLOCKS; i++)
            {
                if (i >= 12)
                {
                    level++;
                    start_offset = LEVEL_OFFSET(level);
                }

                int block_no = curr_in.i_block[i];
                print_indirect(block_no, level, curr_in.i_size, inode_no + 1, 'd');
            }
        }
    }
}

// MAIN
int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        throwError("Wrong number of arguments", 1);
    }

    FILENAME = argv[1];
    offset = BASE_OFFSET;

    if ((fd = open(FILENAME, O_RDONLY)) == -1)
    {
        throwError("Unable to open file", 1);
    }

    print_super_block();
    print_group();
    print_free_blocks();
    print_free_inodes();
    print_inodes();

    return 0;
}
