#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "ext2_fs.h"
#include <sys/types.h>
#include <fcntl.h>
#include <time.h>
#include <stdint.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>

#define SUPER_B_OFFSET 1024
#define Byte_offset 4
int Image_Fd;
struct ext2_super_block Super_block;
struct ext2_group_desc Groups;
struct ext2_inode inode;
struct ext2_dir_entry Directory_Entry;

typedef uint64_t Var64;
typedef uint32_t Var32;

Var32 Total_num_Inode;
Var32 Num_Blocks;
Var32 Num_Inodes;
Var32 Block_size;
Var32 Inode_size;
Var32 First_block_Inodes;
Var32 Free_Inode_BitmapNum;

void print_superblock_summ();
void print_group_summ();
void print_directory_entries(Var32 Offset, Var32 i);
void free_block();
void free_Inode();
void Inode_summ();
void Get_indirect_block_references(Var32 i, Var32 Iblock, int k);

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        fprintf(stderr, "%s\n", "Incorrect arguments provided");
        exit(1);
    }
    //opening the image
    Image_Fd = open(argv[1], O_RDONLY);

    if (Image_Fd == -1)
    {
        fprintf(stderr, "Could not open the image\n");
        exit(2);
    }

    //reading the Super Block information
    ssize_t Pread = pread(Image_Fd, &Super_block, sizeof(struct ext2_super_block), SUPER_B_OFFSET);

    if (Pread < 0)
    {
        fprintf(stderr, "Error in reading Super Block using pread\n");
        exit(2);
    }
    if (Super_block.s_magic != EXT2_SUPER_MAGIC)
    {
        fprintf(stderr, "Bad File System\n");
        exit(2); /* bad file system */
    }

    // Getting and printing the information from the Super Block
    print_superblock_summ();

    ssize_t Pread2 = pread(Image_Fd, &Groups, sizeof(struct ext2_group_desc), (SUPER_B_OFFSET + sizeof(struct ext2_super_block)));

    if (Pread2 < 0)
    {
        fprintf(stderr, "Error in reading Group Desc. using pread\n");
        exit(2);
    }

    print_group_summ();
    //free block entries
    free_block();
    //free I-node entries
    free_Inode();
    //I-node summary
    Inode_summ();
    // Getting the directory information

    for (Var32 i = 0; i < Num_Inodes; i++)
    {

        Var32 Offset = SUPER_B_OFFSET + (First_block_Inodes - 1) * (Block_size) + i * sizeof(struct ext2_inode);
        pread(Image_Fd, &inode, Inode_size, Offset);

        if (inode.i_mode == 0 || inode.i_links_count == 0)
        {
            continue;
        }
        else if (S_ISDIR(inode.i_mode))
        {
            print_directory_entries(Offset, i + 1);
        }

        if (!(S_ISLNK(inode.i_mode)))
        {
            Var32 Indirct_block;
            int k = 0;
            if (inode.i_block[12] != 0)
            {
                Indirct_block = inode.i_block[12];
                k = 12;
                Get_indirect_block_references(i + 1, Indirct_block, k);
            }
            if (inode.i_block[13] != 0)
            {
                Indirct_block = inode.i_block[13];
                k = 13;
                Get_indirect_block_references(i + 1, Indirct_block, k);
            }
            if (inode.i_block[14] != 0)
            {
                Indirct_block = inode.i_block[14];
                k = 14;
                Get_indirect_block_references(i + 1, Indirct_block, k);
            }
        }
    }
}

void print_superblock_summ()
{

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

    Num_Blocks = Super_block.s_blocks_count;
    Num_Inodes = Super_block.s_inodes_count;
    Block_size = SUPER_B_OFFSET << Super_block.s_log_block_size;
    Inode_size = Super_block.s_inode_size;

    Blocks_per_Group = Super_block.s_blocks_per_group;
    Inodes_per_Group = Super_block.s_inodes_per_group;
    First_NonReversed_Inode = Super_block.s_first_ino;

    fprintf(stdout, "SUPERBLOCK,%u,%u,%u,%u,%u,%u,%u\n", Num_Blocks, Num_Inodes, Block_size, Inode_size, Blocks_per_Group, Inodes_per_Group, First_NonReversed_Inode);
}

void print_group_summ()
{

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

    if (Super_block.s_blocks_count < Super_block.s_blocks_per_group)
    {
        Total_num_Blocks = Super_block.s_blocks_count;
    }
    else
    {
        Total_num_Blocks = Super_block.s_blocks_per_group;
    }
    Var32 Total_num_Inodes = Super_block.s_inodes_per_group;
    Var32 Num_free_Bloks = Groups.bg_free_blocks_count;
    Var32 Num_free_Inodes = Groups.bg_free_inodes_count;
    Var32 Free_Block_BitmapNum = Groups.bg_block_bitmap;
    Var32 Free_Inode_BitmapNum = Groups.bg_inode_bitmap;
    First_block_Inodes = Groups.bg_inode_table;
    fprintf(stdout, "GROUP,0,%u,%u,%u,%u,%u,%u,%u\n", Total_num_Blocks, Total_num_Inodes, Num_free_Bloks, Num_free_Inodes, Free_Block_BitmapNum, Free_Inode_BitmapNum, First_block_Inodes);
}
void print_directory_entries(Var32 Offset, Var32 index)
{
    //scan every data block. For each valid (non-zero I-node number) directory entry
    Var32 NodeOffset = Offset + 10*Byte_offset;
    Var32 Block_OFF = Block_size / Byte_offset;
    Var32 j = 0;
    while (j < 12)
    {
        int iblock;
        pread(Image_Fd, &iblock, Byte_offset, NodeOffset + j * Byte_offset);

        if (iblock != 0)
        {
            Var32 OffseChange = Block_size * iblock;
            Var32 Start_Blk = OffseChange;
            while (Start_Blk < OffseChange + Block_size)
            {
                pread(Image_Fd, &Directory_Entry, sizeof(struct ext2_dir_entry), Start_Blk);
                Var32 parent_Inode_num = index;
                Var32 Logical_Byte_offset = Start_Blk - OffseChange;
                Var32 Inode_Number = Directory_Entry.inode;
                Var32 Entry_Length = Directory_Entry.rec_len;
                Var32 Name_Length = Directory_Entry.name_len;
                Start_Blk += Entry_Length;
                if (Inode_Number == 0)
                    continue;
                fprintf(stdout, "DIRENT,%u,%u,%u,%u,%u,'%s'\n", parent_Inode_num, Logical_Byte_offset, Inode_Number, Entry_Length, Name_Length, Directory_Entry.name);
            }
        }
        j++;
    }
    int Level1;
    int iblock2;
    pread(Image_Fd, &Level1, Byte_offset, NodeOffset + 12*Byte_offset);
    if (Level1 != 0)
    {
        Var32 j = 0;
        while (j < Block_OFF)
        {
            Var32 OffSet = Block_size * Level1 + j * Byte_offset;
            pread(Image_Fd, &iblock2, Byte_offset, OffSet);
            int Cur_Offset = Block_size * iblock2;
            Var32 Start_Blk = Cur_Offset;
            if (iblock2 != 0)
            {
                while (Start_Blk < Cur_Offset + Block_size)
                {
                    pread(Image_Fd, &Directory_Entry, sizeof(struct ext2_dir_entry), Start_Blk);
                    Var32 parent_Inode_num = index;
                    Var32 Logical_Byte_offset = Start_Blk - Cur_Offset;
                    Var32 Inode_Number = Directory_Entry.inode;
                    Var32 Entry_Length = Directory_Entry.rec_len;
                    Var32 Name_Length = Directory_Entry.name_len;
                    Start_Blk += Entry_Length;
                    if (Inode_Number == 0)
                        continue;
                    fprintf(stdout, "DIRENT,%u,%u,%u,%u,%u,'%s'\n", parent_Inode_num, Logical_Byte_offset, Inode_Number, Entry_Length, Name_Length, Directory_Entry.name);
                }
            }
            j++;
        }
    }

    int Level2;
    int Level12;
    int iblock3;
    pread(Image_Fd, &Level2, Byte_offset, NodeOffset + 13*Byte_offset);
    if (Level2 != 0)
    {
        Var32 k = 0;
        while (k < Block_OFF)
        {
            Var32 OffSet_L2 = Level2 * Block_size + k * Byte_offset;
            pread(Image_Fd, &Level12, Byte_offset, OffSet_L2);
            if (Level12 != 0)
            {
                Var32 j = 0;

                while (j < Block_OFF)
                {
                    Var32 OffSet_L12 = Block_size * Level12 + j * Byte_offset;

                    pread(Image_Fd, &iblock3, Byte_offset, OffSet_L12);
                    int Cur_Offset = Block_size * iblock3;
                    if (iblock3 != 0)
                    {
                        Var32 Start_Blk = Cur_Offset;
                        while (Start_Blk < Cur_Offset + Block_size)
                        {
                            pread(Image_Fd, &Directory_Entry, sizeof(struct ext2_dir_entry), Start_Blk);
                            Var32 parent_Inode_num = index;
                            Var32 Logical_Byte_offset = Start_Blk - Cur_Offset;
                            Var32 Inode_Number = Directory_Entry.inode;
                            Var32 Entry_Length = Directory_Entry.rec_len;
                            Var32 Name_Length = Directory_Entry.name_len;
                            Start_Blk += Entry_Length;
                            if (Inode_Number == 0)
                                continue;
                            fprintf(stdout, "DIRENT,%u,%u,%u,%u,%u,'%s'\n", parent_Inode_num, Logical_Byte_offset, Inode_Number, Entry_Length, Name_Length, Directory_Entry.name);
                        }
                    }
                    j++;
                }
            }
            k++;
        }
    }
    int Level3;
    int Level23;
    int Level13;
    int iblock4;
    pread(Image_Fd, &Level3, Byte_offset, NodeOffset + 14*Byte_offset);
    if (Level3 != 0)
    {
        Var32 m = 0;
        while (m < Block_OFF)
        {
            Var32 OffSet_L3 = Level3 * Block_size + m * Byte_offset;
            pread(Image_Fd, &Level23, Byte_offset, OffSet_L3);
            if (Level23 != 0)
            {
                Var32 k = 0;
                while (k < Block_OFF)
                {
                    Var32 OffSet_L23 = Level23 * Block_size + k * Byte_offset;
                    pread(Image_Fd, &Level13, Byte_offset, OffSet_L23);
                    if (Level13 != 0)
                    {
                        for (Var32 j = 0; j < Block_OFF; j++)
                        {
                            Var32 OffSet_L13 = Block_size * Level13 + j * Byte_offset;

                            pread(Image_Fd, &iblock4, Byte_offset, OffSet_L13);
                            int Cur_Offset = Block_size * iblock4;
                            if (iblock4 != 0)
                            {

                                Var32 Start_Blk = Cur_Offset;
                                while (Start_Blk < Cur_Offset + Block_size)
                                {
                                    pread(Image_Fd, &Directory_Entry, sizeof(struct ext2_dir_entry), Start_Blk);
                                    Var32 parent_Inode_num = index;
                                    Var32 Logical_Byte_offset = Start_Blk - Cur_Offset;
                                    Var32 Inode_Number = Directory_Entry.inode;
                                    Var32 Entry_Length = Directory_Entry.rec_len;
                                    Var32 Name_Length = Directory_Entry.name_len;
                                    Start_Blk += Entry_Length;
                                    if (Inode_Number == 0)
                                        continue;
                                    fprintf(stdout, "DIRENT,%u,%u,%u,%u,%u,'%s'\n", parent_Inode_num, Logical_Byte_offset, Inode_Number, Entry_Length, Name_Length, Directory_Entry.name);
                                }
                            }
                        }
                    }
                    k++;
                }
            }
            m++;
        }
    }
    //end of function
}
void free_block()
{
    /*
    Scan the free block bitmap for each group. For each free block, produce a new-line terminated line, 
    with two comma-separated fields (with no white space).
    1. BFREE
    2. number of the free block (decimal)
    Take care to verify that you:
    1. understand whether 1 means allocated or free.
    2. have correctly understood the block number to which the first bit corresponds.
    3. know how many blocks are in each group, and do not interpret more bits than there are blocks in the group.
  */
    unsigned int i;
    int j;
    int num_free;
    unsigned char buffer;
    for (i = 0; i < Block_size; i++)
    {
        //https://docs.huihoo.com/doxygen/linux/kernel/3.7/ext2_8h_source.html#l00197
        ssize_t pread_free_block = pread(Image_Fd, &buffer, 1, Block_size * Groups.bg_block_bitmap + i);
        if (pread_free_block < 0)
        {
            fprintf(stderr, "Error in reading in free node\n");
            exit(2);
        }
        for (j = 0; j < 8; j++)
        {
            num_free = (j + 1) + (i * 8); //Please Check this part!!!
            ///////////////////////////////////////////////
            if ((buffer & (1 << j)) == 0)
            {
                fprintf(stdout, "BFREE,%d\n", num_free);
            }
        } // for loop with in j.
    }     //for loop with int i.
}

// 1 indicates the block is used, and 0 indicates the block is free.
void free_Inode()
{
    /*
    Scan the free I-node bitmap for each group. For each free I-node, produce a new-line terminated line, 
    with two comma-separated fields (with no white space).
    1. IFREE
					   2. number of the free I-node (decimal)
					   Take care to verify that you:
    1. understand whether 1 means allocated or free.
    2. have correctly understood the I-node number to which the first bit corresponds.
					   3. know how many I-nodes are in each group, and do not interpret more bits than there are I-nodes in the group.
   */
    unsigned int i;
    int j;
    int num_Ifree;
    unsigned char buffer_I;
    for (i = 0; i < Block_size; i++)
    {
        //https://docs.huihoo.com/doxygen/linux/kernel/3.7/ext2_8h_source.html#l00197
        ssize_t pread_Ifree_block = pread(Image_Fd, &buffer_I, 1, i + Block_size * Groups.bg_inode_bitmap);
        if (pread_Ifree_block < 0)
        {
            fprintf(stderr, "Error in reading in the free I-node entries.\n");
            exit(2);
        }
        for (j = 0; j < 8; j++)
        {
            num_Ifree = i * 8 + j + 1; // index is start at 1.
            if ((buffer_I & (1 << j)) == 0)
            {
                fprintf(stdout, "IFREE,%d\n", num_Ifree);
            }
        } //for loop with int j.
    }     //for loop wiht int i.
}

/*
struct ext2_inode {
   __u16 i_mode ; File mode
   __u16 i_uid ; Owner Uid
   __u32 i_size ; Size in bytes
   __u32 i_atime ; Access time
   __u32 i_ctime ; Creation time
   __u32 i_mtime ; Modification time
   __u32 i_dtime ;* Deletion Time
   __u16 i_gid ;; Group Id
   __u16 i_links_count ; Links count
   …
   __u32 i_block [EXT2_N_BLOCKS];  Pointers to blocks
*/

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//I think you can add the directory entries and indirect block references on here. It will be easy to write the code.
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Inode_summ()
{
    /*
    1.INODE
    2.inode number (decimal)
    3.file type ('f' for file, 'd' for directory, 's' for symbolic link, '?" for anything else) //https://stackoverflow.com/questions/40163270/what-is-s-isreg-and-what-does-it-do
    4.mode (low order 12-bits, octal ... suggested format "%o")
    5.owner (decimal)
    6.group (decimal)
    7.link count (decimal)
    8.time of last I-node change (mm/dd/yy hh:mm:ss, GMT)
    9.modification time (mm/dd/yy hh:mm:ss, GMT)
    10.time of last access (mm/dd/yy hh:mm:ss, GMT)
    11.file size (decimal)
    12.number of (512 byte) blocks of disk space (decimal) taken up by this file
  */
    struct ext2_inode inode;
    unsigned int i;
    //s_inodes_count: 32bit value indicating the total number of inodes, both used and free, in the file system.
    //This value must be lower or equal to (s_inodes_per_group * number of block groups).
    //It must be equal to the sum of the inodes defined in each block group.
    for (i = 0; i < Super_block.s_inodes_count; i++)
    {
        ssize_t pread_Inode_summ = pread(Image_Fd, (void *)&inode, sizeof(inode), (Block_size * Groups.bg_inode_table) + (i * sizeof(inode)));
        if (pread_Inode_summ < 0)
        {
            fprintf(stderr, "Error in reading in the free I-node entries.\n");
            exit(2);
        }
        //https://www.win.tue.nl/~aeb/linux/fs/ext2/ext2.html#I-MODE
        //http://cs.smith.edu/~nhowe/teaching/csc262/oldlabs/ext2.html
        //i_mode: 16bit value used to indicate the format of the described file and the access rights. Here are the possible values,
        //        which can be combined in various ways:
        //        The file format of the i_mode is always non-zero, so we have to check the i_mode value before getting the values
        //
        //i_links_count: 16bit value indicating how many times this particular inode is linked (referred to).
        //               when i_link_count == 0 --> inode is freed.

        //check the value of i_mode and i_links_count.
        /*
    if(inode.i_mode == 0 || inode.i_links_count == 0){
      fprintf(stderr, "i_mode = %d, i_links_count = %d\n", inode.i_mode, inode.i_links_count);
      fprintf(stderr, "Error: Fail to allocate ext2_inode.\n");
      exit(2);
    }
    */
        if (inode.i_mode != 0 && inode.i_links_count != 0)
        {
            char file_type = '?'; // default is ? for anything else instead of file, directory,or symbolic link.
            /*
	Get the type of the file: Use macro defined in <sys/ stats.h >, returns 0 if false, non zero
	if true.
	• S_ISDIR( i_mode ): Test for a directory,
	• S_ISREG( i_mode ): Test for a regular
	• S_ISLNK( i_mode ): Test for a symbolic
      */
            if (S_ISDIR(inode.i_mode))
                file_type = 'd';
            else if (S_ISREG(inode.i_mode))
                file_type = 'f';
            else if (S_ISLNK(inode.i_mode))
                file_type = 's';

            //Print the fields from 1 to 7
            fprintf(stdout, "INODE,%d,%c,%o,%d,%d,%d,",
                    (i + 1),
                    file_type,
                    inode.i_mode & 0XFFF,
                    inode.i_uid,
                    inode.i_gid,
                    inode.i_links_count);

            time_t c_time = inode.i_ctime;
            time_t m_time = inode.i_mtime;
            time_t a_time = inode.i_atime;
            char c_time_format[100]; // Store the time of last I-node change
            char m_time_format[100]; // Store the time of modification time
            char a_time_format[100]; // Store the time of last access.
            struct tm ts;
            //http://man7.org/linux/man-pages/man3/strftime.3.html
            //        strftime - format date and time
            //        size_t strftime(char *s, size_t max, const char *format, const struct tm *tm);

            // 8.  i_ctime: Creation Time. 32bit value representing the number of seconds since january 1st 1970 when the file was created.
            ts = *gmtime(&c_time);
            strftime(c_time_format, sizeof(c_time_format), "%m/%d/%y %H:%M:%S", &ts);

            //9.  i _mtime: Modification Time. 32bit value representing the number of seconds since january 1st 1970 of the last time this file was modified.
            ts = *gmtime(&m_time);
            strftime(m_time_format, sizeof(m_time_format), "%m/%d/%y %H:%M:%S", &ts);

            //10. i_atime: Access Time. 32bit value representing the number of seconds since january 1st 1970 of the last time this file was accessed.
            ts = *gmtime(&a_time);
            strftime(a_time_format, sizeof(a_time_format), "%m/%d/%y %H:%M:%S", &ts);

            //Print the fields from 8 to 11.
            fprintf(stdout, "%s,%s,%s,%u,%d", c_time_format, m_time_format, a_time_format, inode.i_size, inode.i_blocks);

            //12.number of (512 byte) blocks of disk space (decimal) taken up by this file
            unsigned int block_index;
            //EXT2_N_BLOCKS --> from ext2_fs.h
            for (block_index = 0; block_index < 15; block_index++)
            {
                fprintf(stdout, ",%d", inode.i_block[block_index]);
            } //for loop with i_block.
            fprintf(stdout, "\n");
        } //for loop with int i.
    }
}

void Get_indirect_block_references(Var32 i, Var32 Iblock, int k)
{
    if (k == 12)
    {
        Var32 blockOffset = Iblock * Block_size;
        Var32 Block_Value;
        Var32 block_off = Block_size / Byte_offset;
        for (Var32 j = 0; j < block_off; j++)
        {
            pread(Image_Fd, &Block_Value, sizeof(Block_Value), blockOffset + j * Byte_offset);
            Var32 Logical_Offset = 12 + j;
            if (Block_Value != 0)
                fprintf(stdout, "INDIRECT,%u,%u,%u,%u,%u\n", i, 1, Logical_Offset, Iblock, Block_Value);
        }
    }
    if (k == 13)
    {
        Var32 Blocke_entry_L1Indirect = Iblock * Block_size;
        Var32 Block_Offset;
        Var32 Blocke_Value_L1;
        Var32 Block_Value;
        Var32 block_off = Block_size / Byte_offset;
        Var32 j = 0;
        while ( j < block_off)
        {
            pread(Image_Fd, &Blocke_Value_L1, sizeof(Block_Value), Blocke_entry_L1Indirect + j * Byte_offset);

            Var32 Logical_offset_L2 = 12 + block_off + j * block_off;
            Block_Offset = Blocke_Value_L1 * Block_size;
            if (Blocke_Value_L1 != 0)
            {
                fprintf(stdout, "INDIRECT,%u,%u,%u,%u,%u\n", i, 2, Logical_offset_L2, Iblock, Blocke_Value_L1);
                for (Var32 k = 0; k < block_off; k++)
                {
                    Var32 Logical_offset_L1 = Logical_offset_L2 + k;
                    pread(Image_Fd, &Block_Value, sizeof(Block_Value), Block_Offset + k * Byte_offset);

                    if (Block_Value != 0)
                        fprintf(stdout, "INDIRECT,%u,%u,%u,%u,%u\n", i, 1, Logical_offset_L1, Blocke_Value_L1, Block_Value);
                }
            }
            j++;
        }
    }
    if (k == 14)
    {
        Var32 Blocke_entry_L3 = Iblock * Block_size;
        Var32 block_offSet = Block_size / Byte_offset;
        Var32 Block_entry_offset = 12 + block_offSet * (1 + block_offSet);
        Var32 a = 0; 
        while (a < block_offSet )
        {
            //Get each entry in Level 3
            Var32 Block_entry_L2, Block_Numb_L2;
            pread(Image_Fd, &Block_Numb_L2, sizeof(Block_Numb_L2), Blocke_entry_L3 + a * Byte_offset);

            Block_entry_L2 = Block_Numb_L2 * Block_size;
            Var32 Logical_Offset_L3 = Block_entry_offset + a * (Block_size * Block_size / (2*Byte_offset));
            if (Block_Numb_L2 != 0)
            {
                fprintf(stdout, "INDIRECT,%u,%u,%u,%u,%u\n", i, 3, Logical_Offset_L3, Iblock, Block_Numb_L2);
                Var32 j = 0;
                while (j < block_offSet)
                {
                    //Get each entry in Level 2
                    Var32 Blocke_entry_L1, Blocke_Numb_L1;
                    pread(Image_Fd, &Blocke_Numb_L1, sizeof(Blocke_Numb_L1), Block_entry_L2 + j * Byte_offset);

                    Blocke_entry_L1 = Blocke_Numb_L1 * Block_size;
                    if (Blocke_Numb_L1 != 0)
                    {
                        Var32 Logical_Offset_L2 = Logical_Offset_L3 + j * block_offSet;
                        fprintf(stdout, "INDIRECT,%u,%u,%u,%u,%u\n", i, 2, Logical_Offset_L2, Block_Numb_L2, Blocke_Numb_L1);
                        for (Var32 d = 0; d < block_offSet; d++)
                        {
                            Var32 Logical_Offset_L1 = Logical_Offset_L2 + d;
                            Var32 Ref_Block_Numb;
                            //get each entry in level 1
                            pread(Image_Fd, &Ref_Block_Numb, sizeof(Ref_Block_Numb), Blocke_entry_L1 + d * Byte_offset);

                            if (Ref_Block_Numb != 0)
                                fprintf(stdout, "INDIRECT,%u,%u,%u,%u,%u\n", i, 1, Logical_Offset_L1, Blocke_Numb_L1, Ref_Block_Numb);
                        }
                    }
                    j++;
                }
            }
            a++;
        }
    }
}

