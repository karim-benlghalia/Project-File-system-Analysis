#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "ext2_fs.h"
#include <sys/types.h>
#include <fcntl.h>
#include <time.h>
#include <stdint.h>

#define SUPER_B_OFFSET 1024


int Image_Fd;
struct ext2_super_block Super_block;
struct ext2_group_desc Groups;

typedef uint64_t Var64;
typedef uint32_t Var32;
typedef uint16_t Var16;
typedef uint8_t  Var8;

Var32 Num_Blocks;
Var32 Num_Inodes;
Var32 Block_size;
Var32 Inode_size;
void print_superblock_summ();
void print_group_summ();

int main(int argc, char** argv){
    if(argc != 2){
        fprintf(stderr, "%s\n", "Incorrect arguments provided");
        exit(1);
    }
    //opening the image
    Image_Fd = open(argv[1], O_RDONLY);

    if(Image_Fd == -1)
    {
        fprintf(stderr,  "Could not open the image\n" );
        exit(2);
    }

    //reading the Super Block information
    ssize_t Pread =pread(Image_Fd, &Super_block, sizeof(struct ext2_super_block), SUPER_B_OFFSET );

    if(Pread < 0){
        fprintf(stderr, "Error in reading Super Block using pread\n");
		exit(2);
    }
    if (Super_block.s_magic != EXT2_SUPER_MAGIC)
	        exit(2); /* bad file system */	
   
 // Getting and printing the information from the Super Block 
  print_superblock_summ();
 
 ssize_t Pread2 =pread(Image_Fd, &Groups, sizeof(struct ext2_group_desc), (SUPER_B_OFFSET + sizeof(struct ext2_super_block)));

    if(Pread2 < 0){
        fprintf(stderr, "Error in reading Group Desc. using pread\n");
		exit(2);
    }
// Getting and printing the group summary
 print_group_summ();
 


}

void print_superblock_summ(){

    /* Getting and printing the information from the Super Block 
    *SUPERBLOCK
    *total number of blocks (decimal)
    *total number of i-nodes (decimal)
    *block size (in bytes, decimal)
    *i-node size (in bytes, decimal)
    *blocks per group (decimal)
    *i-nodes per group (decimal)
    *first non-reserved i-node (decimal)
 */
Var32 Blocks_per_Group;
Var32 Inodes_per_Group;
Var32 First_NonReversed_Inode;

Num_Blocks=Super_block.s_blocks_count;
Num_Inodes=Super_block.s_inodes_count;
Block_size=1024<<Super_block.s_log_block_size;
Inode_size=Super_block.s_inode_size;

Blocks_per_Group=Super_block.s_blocks_per_group;
Inodes_per_Group=Super_block.s_inodes_per_group;
First_NonReversed_Inode=Super_block.s_first_ino;

fprintf(stdout, "SUPERBLOCK,%u,%u,%u,%u,%u,%u,%u\n", Num_Blocks, Num_Inodes, Block_size, Inode_size, Blocks_per_Group, Inodes_per_Group, First_NonReversed_Inode);
}

void print_group_summ(){

 /* Getting and printing the group summary
    *GROUP
    *group number (decimal, starting from zero) (Since the we have only one group. So group Number=0)
    *total number of blocks in this group (decimal)
    *total number of i-nodes in this group (decimal)
    
    *number of free blocks (decimal)
    *number of free i-nodes (decimal)
    *block number of free block bitmap for this group (decimal)
    *block number of free i-node bitmap for this group (decimal)
    *block number of first block of i-nodes in this group (decimal)
 */
Var32 Total_num_Blocks;
//Based on EXT2 Specification, s_blocks_count must be lower or equal to (s_blocks_per_group * number of block groups). It can be lower 
//than the previous calculation if the last block group has a smaller number of blocks than s_blocks_per_group du to volume size. 
//It must be equal to the sum of the blocks defined in each block group.

if(Super_block.s_blocks_count<Super_block.s_blocks_per_group){
    Total_num_Blocks=Super_block.s_blocks_count;
}
else{
    Total_num_Blocks=Super_block.s_blocks_per_group;
}
Var32 Total_num_Inodes = Super_block.s_inodes_per_group;
Var32 Num_free_Bloks=Groups.bg_free_blocks_count;
Var32 Num_free_Inodes=Groups.bg_free_inodes_count;
Var32 Free_Block_BitmapNum =Groups.bg_block_bitmap;
Var32 Free_Inode_BitmapNum=Groups.bg_inode_bitmap;
Var32 First_block_Inodes=Groups.bg_inode_table;
fprintf(stdout, "GROUP,0,%u,%u,%u,%u,%u,%u,%u\n", Total_num_Blocks, Total_num_Inodes,Num_free_Bloks,Num_free_Inodes, Free_Block_BitmapNum, Free_Inode_BitmapNum, First_block_Inodes);

}


