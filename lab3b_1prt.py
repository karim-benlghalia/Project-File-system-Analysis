#!/usr/bin/python
import sys,csv,math

Dup_blocks=set() # list for duplicate blocks
superblock_argument=0
Group_Argument=0
Allocated_blocks={} # dictionary for allocated block 
inode_argument=[]
dirent_argument=[]
indirect_argument=[]
Free_Block_list=[]
IFREE_argument=[]
Allocated_Inodes=[]
# constants
MAX_B_COUNT =0
First_Block_Numb=0
Const_offset1 = 12
Const_offset2 = 268
Const_offset3 = 65804
#class for superblock
class superblock:
    def __init__(self,argument):
        self.s_blocks_count     =int(argument[1])
        self.s_inodes_count     =int(argument[2])
        self.Block_Size         =int(argument[3])
        self.s_inode_size       =int(argument[4])
        self.s_first_inode        =int(argument[7])

#class for inode
class inode:
    def __init__(self,argument):
        self.Inode_Numb         =int(argument[1])
        self.File_Type          =argument[2]
        self.i_Links_Count      =int(argument[6])
        self.i_size             =int(argument[10])
        self.DIRENT_Blocks      =[int(argument[i]) for i in range(12,24)]
        self.INDIRECT_Blocks    =[int(argument[i]) for i in range(24,27)]

#class for group
class group:
    def __init__(self,argument):
        self.s_inodes_per_group = int(argument[3])
        self.bg_inode_table = int(argument[len(argument)-1])

#class for dirent
class dirent:
    def __init__(self,argument):
        self.Inode_Numb          =int(argument[1])
        self.inode              =int(argument[3])
        self.name               =argument[6]

#class for indirect
class indirect:
    def __init__(self,argument):
        self.Inode_Numb          =int(argument[1])
        self.level              =int(argument[2])
        self.offset             =int(argument[3])
        self.argument            =int(argument[5])

#Check for allocated and duplicate blocks
def find_usedBlocks(inode,Numb_Block,offset,level):
    global Allocated_blocks
    global Dup_blocks
    if Numb_Block != 0:
        if Numb_Block not in Allocated_blocks: 
            Allocated_blocks[Numb_Block] = [[inode.Inode_Numb, offset,level]]
        else:
            Dup_blocks.add(Numb_Block)
            Allocated_blocks[Numb_Block].append([inode.Inode_Numb, offset,level])

#scan inode table
def Scan_Inode_tbl(inode):
        off_set = 0
        i=0
        while i < len(inode.DIRENT_Blocks):
            Numb_Block=inode.DIRENT_Blocks[i]
            if Numb_Block < 0 or Numb_Block >= MAX_B_COUNT:
                print("INVALID BLOCK %d IN INODE %d AT OFFSET %d" %(Numb_Block, inode.Inode_Numb, off_set))
                break
            if Numb_Block > 0:
                if Numb_Block < First_Block_Numb:
                    print("RESERVED BLOCK %d IN INODE %d AT OFFSET %d" %(Numb_Block, inode.Inode_Numb, off_set))
                    return
            
            find_usedBlocks(inode,Numb_Block,off_set,0)
            off_set += 1
            i +=1

# check for single indirect blocks
def check_single_indBlock(inode):
    Numb_Block=inode.INDIRECT_Blocks[0]
    SingleMsg = "INDIRECT BLOCK"
    if inode.INDIRECT_Blocks[0] < 0 or inode.INDIRECT_Blocks[0] >= MAX_B_COUNT:
        print("INVALID %s %d IN INODE %d AT OFFSET %d" %(SingleMsg,Numb_Block, inode.Inode_Numb, Const_offset1))
    if Numb_Block > 0:
        if Numb_Block < First_Block_Numb:
            print("RESERVED %s %d IN INODE %d AT OFFSET %d" %(SingleMsg,Numb_Block, inode.Inode_Numb, Const_offset1))
    find_usedBlocks(inode,Numb_Block,Const_offset1,1)

# check for double indirect blocks
def check_double_indBlock(inode):
    Numb_Block=inode.INDIRECT_Blocks[1]
    BoubleMsg="DOUBLE INDIRECT BLOCK"
    if inode.INDIRECT_Blocks[1] < 0 or inode.INDIRECT_Blocks[1] >= MAX_B_COUNT:
        print("INVALID %s %d IN INODE %d AT OFFSET %d" %(BoubleMsg,Numb_Block, inode.Inode_Numb, Const_offset2))
    if Numb_Block > 0:
        if Numb_Block < First_Block_Numb:
            print("RESERVED %s %d IN INODE %d AT OFFSET %d" %(BoubleMsg,Numb_Block, inode.Inode_Numb, Const_offset2))
    find_usedBlocks(inode,Numb_Block,Const_offset2,2)

# check for triple indirect blocks
def check_triple_indBlock(inode):
    Numb_Block=inode.INDIRECT_Blocks[2]
    TripleMsg="TRIPLE INDIRECT BLOCK"
    if inode.INDIRECT_Blocks[2] < 0 or inode.INDIRECT_Blocks[2] >= MAX_B_COUNT:
        print("INVALID %s %d IN INODE %d AT OFFSET %d" %(TripleMsg,Numb_Block, inode.Inode_Numb, Const_offset3))
    if Numb_Block > 0:
        if Numb_Block < First_Block_Numb:
            print("RESERVED %s %d IN INODE %d AT OFFSET %d" %(TripleMsg,Numb_Block, inode.Inode_Numb, Const_offset3))
    find_usedBlocks(inode,Numb_Block,Const_offset3,3)


def switching(argument):
    switcher = { 
        1: "INDIRECT", 
        2: "DOUBLE INDIRECT", 
        3: "TRIPLE INDIRECT", 
    } 
    return switcher.get(argument, "")


def Check_indirct_blocks():
    i=0
    while i < len(indirect_argument):
        indirect=indirect_argument[i]
        Numb_Block = indirect.argument
        if Numb_Block < 0 or Numb_Block >= MAX_B_COUNT:
            print("INVALID %s BLOCK %d IN INODE %d AT OFFSET %d" %(switching(indirect.level), Numb_Block, indirect.Inode_Numb, indirect.offset))
        if Numb_Block > 0:
            if Numb_Block < First_Block_Numb:
                print("RESERVED %s BLOCK %d IN INODE %d AT OFFSET %d" %(switching(indirect.level), Numb_Block, indirect.Inode_Numb, indirect.offset))
                return
        find_usedBlocks(indirect,Numb_Block,indirect.offset,indirect.level)
        i+=1
 # check for allocated blocks      
def check_Block_alloc():
    i=First_Block_Numb
    while i < MAX_B_COUNT:
        if i not in Free_Block_list:
            if i not in Allocated_blocks:
                print("UNREFERENCED BLOCK %d" %i)
        if i in Free_Block_list:
            if i in Allocated_blocks:
                print("ALLOCATED BLOCK %d ON FREELIST" %i)
        i+=1
# check for duplicate blocks 
def check_Dup_blocks():
    #set(Dup_blocks)
    for i in Dup_blocks:
        for index in Allocated_blocks[i]:
            indirect = switching(index[-1])
            if indirect == "":
                DuplMsg="DUPLICATE BLOCK"
                print("%s %d IN INODE %d AT OFFSET %d" %(DuplMsg,i, index[0], index[1]))
            else:
                DuplMsg="DUPLICATE"
                print("%s %s BLOCK %d IN INODE %d AT OFFSET %d" %(DuplMsg,indirect, i, index[0], index[1]))

def check_inode_alloc(inode):
    global Allocated_Inodes
    if inode.Inode_Numb != 0:
        Allocated_Inodes.append(inode.Inode_Numb)
        if inode.Inode_Numb in IFREE_argument:
            AllocMsg="ALLOCATED INODE"
            print("%s %d ON FREELIST" %(AllocMsg,inode.Inode_Numb))
            
def check_inode_Unalloc(inode):
    #if inode.File_Type == '0':
    #if inode.Inode_Numb not in IFREE_argument:
     if inode.Inode_Numb not in Allocated_Inodes:
        AllocMsg="UNALLOCATED INODE"
        print("%s %d NOT ON FREELIST" %(AllocMsg,inode.Inode_Numb)) 
    



def main ():

    if len(sys.argv) != 2:
        sys.stderr.write("Usage: ./lab3b File.csv \n")
        sys.exit(1)
        
    #open the CSV file
    try:
        input_csvfile=open(sys.argv[1], 'r')
    except IOError:
        sys.stderr.write("Error: cannot open the CSV file \n")
        sys.exit(1)

    #read the csv file
    file_reader=csv.reader(input_csvfile)
    for Data in file_reader:
        if Data[0] == "SUPERBLOCK":
            superblock_argument=superblock(Data)
        elif Data[0] == "GROUP":
            Group_Argument=group(Data)
        elif Data[0] == "BFREE":
            Free_Block_list.append(int(Data[1]))
        elif Data[0] == "IFREE":
            IFREE_argument.append(int(Data[1]))
        elif Data[0] == "INODE":
            inode_argument.append(inode(Data))
        elif Data[0] == "DIRENT":
            dirent_argument.append(dirent(Data))
        elif Data[0] == "INDIRECT":
            indirect_argument.append(indirect(Data))
        else:
            sys.stderr.write("Error: Invild argument in CSV file: \n")
            sys.exit(1)

    #Max number of block
    global MAX_B_COUNT
    MAX_B_COUNT =superblock_argument.s_blocks_count
    
    #index of the first block
    global First_Block_Numb
    N_offset = Group_Argument.s_inodes_per_group * superblock_argument.s_inode_size
    G_offset = N_offset / superblock_argument.Block_Size
    First_Block_Numb = int(Group_Argument.bg_inode_table + G_offset)
    First_Inode=superblock_argument.s_first_inode
    Inodes_Counts=Inodes_Counts=superblock_argument.s_inodes_count

    

    for inodes in inode_argument:
        if inodes.File_Type == 's':
            if inodes.i_size <=MAX_B_COUNT:
                continue
        Scan_Inode_tbl(inodes)
        # check for the single indirect block
        check_single_indBlock(inodes)
        # check for  the double indirect block
        check_double_indBlock(inodes)
        # check for the triple indirect block
        check_triple_indBlock(inodes)
         #scan the inode
        check_inode_alloc(inodes)
         #scan the inode
        check_inode_Unalloc(inodes)

    Check_indirct_blocks()
    check_Block_alloc()
    check_Dup_blocks()
    
    for InodeN in range(First_Inode,Inodes_Counts):
        if InodeN  not in Allocated_Inodes:
            if InodeN  not in IFREE_argument:
                print("UNALLOCATED INODE %d NOT ON FREELIST" %(InodeN))
   



if __name__ == '__main__':
    main()
