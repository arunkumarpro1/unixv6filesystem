//***************************************************************************************************
//OPERATING SYSTEMS AND CONCEPTS PROJECT II
//ARUN KUMAR BALASUBRAMANI     UTD ID 2021335283
//VISHAL IYER                  UTD ID 2021331899
//***************************************************************************************************
// VIM editor
//*----------------------------------------------------------------------------------------------------
//Download and install putty from http://www.chiark.greenend.org.uk/~sgtatham/putty/download.html      -
//connect to cs1.utdallas.edu or cs2.utdallas.edu using the putty program or ssh                                           -
//save this file as name.c, compile using command cc name.c, execute using ./a.out
//Our Program supports the following command              -
//Then give fsacces v6
//initfs 5000 400
//cpin name1.txt name2.txt
//cpout name2.txt name3.txt
//mkdir directory name
//Rm filename
//q to quit the terminal

//----------------------------------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include<limits.h>
#include <sys/stat.h>

#define BUFFERSIZE 200

/** Function Prototype Declarations*/
unsigned short readFreeBlock();
void addFreeBlock(int);
long getFreeInode(int);
void populateSize(long);
int splitCommand(char**, char*, char*);
void cpin(char*, int, int);
void cpout(char*, int);
long getSize();
int searchFileInPath(int,int,char* []);
void mkdirectory(char*, int, int);
void removev6file(char*, int);
void resetinode();
int iteratePath(int,int,char*[]);
void removefileGenericMethod(char*, int);

/** Structure for super block*/
struct superBlock
{
    unsigned short isize;
    unsigned short fsize;
    unsigned short nfree;
    unsigned short free[100];
    unsigned short ninode;
    unsigned short inode_arr[100];
    char flock;
    char ilock;
    char fmod;
    unsigned short time[2];
} super;

/** Structure for i-node*/
struct i_node
{
    unsigned short flags;
    char nlinks;
    char uid;
    char gid;
    char size0;
    unsigned short size1;
    unsigned short addr[8];
    unsigned short acttime[2];
    unsigned short modtime[2];
} inode;

/** Structure for entries in the directory block*/
struct dir_entry
{
    unsigned short inode_num;
    char filename[14];
} dir;

/** Global variable for file descriptor of the v6 file system*/
int fd;

const long maxsize = 0x2000000;  //32MB

int main()
{
//**** file system initilization function************/
    char *command = malloc(200);
    fgets(command, BUFFERSIZE, stdin);
    command[strcspn(command, "\n")] = 0;
    char *fsacess = "fsaccess";
    int i, n_blocks, n_inodes, n_inode_blocks, d_start;
    char *part[3];
    if (strncmp(fsacess, command, 8) == 0)
    {
        char* fullpath = malloc(100);
        splitCommand(part, command, " ");
        strcpy(fullpath,part[1]);
        if (strncmp(part[1], "/", 1) == 0)  // comparing initfs with the recieved command
        {
            char* home = getenv("HOME");
            if (home == NULL) return 1;

            size_t len = strlen(home) + strlen(part[1]) + 1;
            fullpath = malloc(len);
            if (fullpath == NULL) return 1;

            strcpy(fullpath, home);
            strcat(fullpath, part[1]);
        }
//*****open the file as readandwrite,create the file if not there,clear all data file and give read,write and execution permission*************
        fd = open(fullpath, O_RDWR | O_CREAT | O_TRUNC, S_IREAD | S_IEXEC | S_IWRITE);
        if (fd < 0)
        {
            perror("Unable to open file system..!");
            return 1;
        }
        for (;;)
        {
            fgets(command, BUFFERSIZE, stdin);     // taking fgets command as input
            command[strcspn(command, "\n")] = 0;
            if (strncmp("initfs", command, 6) == 0)  // comparing initfs with the recieved command
            {
                splitCommand(part, command, " ");   // splitting the command with space delimiter

                n_blocks = atoi(part[1]);  //convert nblocks from string to integer
                n_inodes = atoi(part[2]);  // convert ninodes from string to integer
                n_inode_blocks = n_inodes / 16; // inodes are 32bytes long so 16 fit into 1 block
                if(n_inodes%16>0)
                    n_inode_blocks+=1;
                d_start = n_inode_blocks + 2;  // data blocks start after total inodes blocks plus two,leaving one for
                //  bootloader and one for superblock

                super.isize = n_inodes / 16; // total number of blocks to ilist
                super.fsize = n_blocks;      // fsize is the limit for data blocks
                super.free[0] = 0;           // initialisation of free aray
                super.nfree = 1;             // nfree is 1

                for (i = d_start + 1; i < n_blocks; i++)
                {
                    addFreeBlock(i);         // adding free blocks for all total number of blocks
                }
                // lseek() function allows the file offset to be set beyond the end of the file
                lseek(fd, 512, SEEK_SET);
                write(fd, &super, sizeof(struct superBlock)); // writing of super block

                inode.flags = 0xcdff; // allocated,directory type file,set user and group id,give all users read,write and exec access
                inode.nlinks = 1;// link to the file
                inode.uid = 0;
                inode.gid = 0;
                // copies 0 to the first n characters of the string pointed to by the argument addr.
                memset(inode.addr, 0, sizeof(inode.addr));
                inode.addr[0] = d_start; // assigning address array to data blocks
                populateSize(32);

                //writing root i-node
                lseek(fd, 1024, SEEK_SET);
                write(fd, &inode, sizeof(struct i_node));

                //writing the remaining i-nodes
                for (i = 0; i < n_inodes - 1; i++)
                {
                    inode.flags = 0x0dff;// unallocated,plain and small file,set user and group id,give all users read,write and exec access
                    inode.nlinks = 0;  // destroy the file
                    write(fd, &inode, sizeof(struct i_node));
                }

                //**********************writing the data block for root directory*******************************
                dir.inode_num = 1;
                strcpy(dir.filename, ".\0");
                lseek(fd, d_start * 512, SEEK_SET);
                write(fd, &dir, sizeof(struct dir_entry));  // first entry of the directory

                dir.inode_num = 1;
                strcpy(dir.filename, "..\0");
                write(fd, &dir, sizeof(struct dir_entry)); // second entry of directory

                dir.inode_num = 0;
                strcpy(dir.filename, "\0");
                for (i = 0; i < 14; i++)
                    write(fd, &dir, sizeof(struct dir_entry)); // writing the remaining entry of directory

            }
            else if (strncmp("cpin", command, 4) == 0)  // comparing command with cpin
            {
                cpin(command, n_inodes, d_start);
            }
            else if (strncmp("cpout", command, 5) == 0) // comparing command with cpout
            {
                cpout(command, d_start);
            }
            else if (strncmp("mkdir", command, 5) == 0) // comparing command with mkdir
            {
                mkdirectory(command, d_start, n_inodes);
            }
            else if (strncmp("Rm", command, 2) == 0)    // comparing command with Remove Rm
            {
                removev6file(command, d_start);
            }
            else if (strcmp("q", command) == 0)          // comparing command with quit q
            {
                printf("Exiting the file system.");
                break;
            }
            else
            {
                printf("Incorrect Command.. Please try again..!\n");
            }
        }
    }
    else
    {
        printf("Incorrect command to enter the file system...Please try again.\n");
    }
    return 0;
}
/***************** function to split command into tokens*****************************/
int splitCommand(char **part, char* command, char* split)
{
    int i = 0;
    part[i] = strtok(command, split);
    while (part[i] != NULL)
    {
        i++;
        part[i] = strtok(NULL, split);  // splitting command with null and storing it in parts array
    }
    return i;
}

/*********** free data blocks and initialize free array ************/
void addFreeBlock(int block_no)
{
    if (super.nfree == 100)
    {
        lseek(fd, block_no * 512, SEEK_SET);// go to particular block number
        write(fd, &(super.nfree), sizeof(unsigned short)); // copy nfree into first 2 bytes of the block
        write(fd, super.free, super.nfree*sizeof(unsigned short));// copy free array into the next nfree bytes
        super.nfree=0;// Reset nfree to 0
    }
    super.free[super.nfree] = block_no;  //set free(nfree) to block number
    super.nfree++;
}


/**********function to get a free data block. Also decrements nfree for each pass*********/
unsigned short readFreeBlock()
{
    int i;
    super.nfree--;    // decrement nfree
    unsigned short freeBlock = super.free[super.nfree];// new block is free(nfree)
    if (super.nfree == 0)
    {
        if (freeBlock == 0) // is free block is 0 then error no memory available
        {
            super.nfree++;
            printf("No free memory blocks available...\n");
        }
        else
        {
            lseek(fd, freeBlock * 512, SEEK_SET); //go to  new freeblock
            read(fd, &(super.nfree), sizeof(unsigned short));//
            for (i = 0; i < super.nfree; i++)
            {
                read(fd, &(super.free[i]), sizeof(unsigned short));// replace nfree by first word amd copy block numbers in the next
                //100 words pf free array
            }
        }
    }
    return freeBlock;
}

/*********************populate size of the file*************/
void populateSize(long size)
{
    long c = 24, k;

    k = size >> c; // Division by 2 power 24
    inode.size0 = 0;
    inode.size1 = 0;
    if (k & 1)          // To check whether the result of division is 1
        inode.flags |= 1 << 9;    // setting the 9th bit and using it as msb of size
    else
        inode.flags &= ~(1 << 9); //clear the 9th bit of flag

    for (c = 23; c >= 16; c--)
    {
        k = size >> c;

        if (k & 1)
            inode.size0 |= 1 << (c - 16);
        else
            inode.size0 |= 0 << (c - 16);
    }

    for (c = 15; c >= 0; c--)
    {
        k = size >> c;

        if (k & 1)
            inode.size1 |= 1 << c;
        else
            inode.size1 |= 0 << c;
    }
}


/******** Get the size of the file stored in 9th bit of flags, size0, size1 variables of inode ***********/
long getSize()
{
    int c;
    long total = 0;
    total |= ((inode.flags >> 9) & 1) << 24;
    for (c = 23; c >= 16; c--)
    {
        total |= ((inode.size0 >> (c - 16)) & 1) << c;
    }
    for (c = 15; c >= 0; c--)
    {
        total |= ((inode.size1 >> c) & 1) << c;
    }
    return total;
}


/*************** Function to allocate free inode****************/
long getFreeInode(int n_inodes)
{
    int i;
    lseek(fd, 1024, SEEK_SET);
    for (i = 1; i <= n_inodes; i++)
    {
        read(fd, &(inode), sizeof(struct i_node));
        if (!((inode.flags >> 15) & 1))
            break;
    }
    return i;
}


/************** Function to copy file contents from external file to v6 filesystem************/
void cpin(char *command, int n_inodes, int d_start)
{
    char *part[3];
    long fileSize = 0;
    splitCommand(part, command, " ");
    int i, in_read, out_read, inode_i,index = 0;
    int sourceFileDes = open(part[1], O_RDONLY, S_IREAD);
    char *filepath[100];
    int lengthOfPath = splitCommand(filepath, part[2], "/");
    unsigned short freeblock, indirectblock,addr_i,offset;
    if (sourceFileDes < 0)
    {
        perror("Unable to open file!");
        return;
    }

    int in, free_inode = getFreeInode(n_inodes);                // Get an inode which is unallocated
    char buffer[512];


//Write to the directory

    lseek(fd, 1024, SEEK_SET);
    read(fd, &(inode), sizeof(struct i_node));                  //Read the inode of the root directory

    d_start = iteratePath(d_start,lengthOfPath,filepath);       //Iterate through the given path


//Check whether a file with the same name already exists in the given location
    lseek(fd, d_start * 512, SEEK_SET);
    for (i = 0; i < 32; i++)
    {
        read(fd, &dir, sizeof(struct dir_entry));
        if (dir.inode_num!=0 && strcmp(dir.filename,filepath[lengthOfPath-1]) == 0)
        {
            printf("File with the same name already exists..!");
            return;
        }

    }

    for(inode_i=0; inode_i<8; inode_i++)
    {
        if(inode.addr[inode_i]==0)
        {
            inode.addr[inode_i]=readFreeBlock();
            if(inode.addr[inode_i]==0)
                return;
        }
        lseek(fd, inode.addr[inode_i] * 512, SEEK_SET);
        for (i = 0; i < 32; i++)
        {
            read(fd, &dir, sizeof(struct dir_entry));
            if (dir.inode_num == 0)
                goto OUTSIDE;
        }
    }

OUTSIDE:
    dir.inode_num = free_inode;
    strcpy(dir.filename, filepath[lengthOfPath-1]);
    lseek(fd, -16, SEEK_CUR);
    write(fd, &dir, sizeof(struct dir_entry));

//preparing the inode for the file
    inode.nlinks = 1;
    inode.uid = 0;
    inode.gid = 0;
    memset(inode.addr, 0, sizeof(inode.addr));     // Assign 0 to all the addr elements
    inode.flags |= 1 << 15;                        // Set the allocation bit of inode

    while ((in_read = read(sourceFileDes, &buffer, sizeof(buffer))) > 0)
    {
        if((freeblock = readFreeBlock())==0)
        {
            removefileGenericMethod(filepath[lengthOfPath-1],d_start);
            return;
        }
        lseek(fd, freeblock * 512, SEEK_SET);
        out_read = write(fd, &buffer, in_read);
        fileSize += out_read;

        if(fileSize>=maxsize)
        {
            printf("Size of the file greater than 32MB\n");
            removefileGenericMethod(filepath[lengthOfPath-1],d_start);
            return;
        }
        if (index == 8) //Transform a small file to large file
        {
            if((indirectblock = readFreeBlock())==0)
            {
                removefileGenericMethod(filepath[lengthOfPath-1],d_start);
                return;
            }
            lseek(fd, indirectblock * 512, SEEK_SET);
            for (in = 0; in < 8; in++)
                write(fd, &(inode.addr[in]), sizeof(unsigned short));
            write(fd, &(freeblock), sizeof(unsigned short));
            inode.addr[0] = indirectblock;
            //Set the 12 th bit of flag to large file
            inode.flags |= 1 << 12;
        }
        else if (index > 8)   // large file
        {
            addr_i = index >> 8;
            offset = index & 255;
            if (offset == 0)      //offset==0  => new block must be created
            {
                //huge (extra large) file
                if (addr_i == 7)
                {
                    indirectblock = readFreeBlock();
                    unsigned short secondIndirectblock = readFreeBlock();
                    lseek(fd, indirectblock * 512, SEEK_SET);
                    write(fd, &(secondIndirectblock), sizeof(unsigned short));
                    lseek(fd, secondIndirectblock * 512, SEEK_SET);
                    write(fd, &(freeblock), sizeof(unsigned short));
                    inode.addr[addr_i] = indirectblock;
                }
                else if (addr_i > 7)
                {
                    unsigned short secondIndirectblock = readFreeBlock();
                    lseek(fd, (inode.addr[7] * 512)+((addr_i-7)*sizeof(unsigned short)), SEEK_SET);
                    write(fd, &(secondIndirectblock), sizeof(unsigned short));
                    lseek(fd, secondIndirectblock * 512, SEEK_SET);
                    write(fd, &(freeblock), sizeof(unsigned short));
                }
                else
                {
                    indirectblock = readFreeBlock();
                    lseek(fd, indirectblock * 512, SEEK_SET);
                    write(fd, &(freeblock), sizeof(unsigned short));
                    inode.addr[addr_i] = indirectblock;
                }
            }
            else
            {
                //huge (extra large) file
                if (addr_i >= 7)
                {
                    unsigned short index2 = index - (7 * 256);
                    unsigned short i2 = index2 >> 8;
                    unsigned short offset2 = index2 & 255;
                    int bl = (inode.addr[7] * 512)
                             + (i2 * sizeof(unsigned short));
                    lseek(fd, bl, SEEK_SET);
                    unsigned short secondIndirectblock;
                    read(fd, &(secondIndirectblock), sizeof(unsigned short));
                    lseek(fd,
                          (secondIndirectblock * 512)
                          + (offset2 * sizeof(unsigned short)),
                          SEEK_SET);
                    write(fd, &(freeblock), sizeof(unsigned short));
                }
                else  // large file
                {
                    lseek(fd,
                          (inode.addr[addr_i] * 512)
                          + (offset * sizeof(unsigned short)),
                          SEEK_SET);
                    write(fd, &(freeblock), sizeof(unsigned short));
                }
            }
        }
        else      // small file
            inode.addr[index] = freeblock;
        index++;
    }

    //write the modified inode
    populateSize(fileSize);
    lseek(fd, 1024 + ((free_inode - 1) * 32), SEEK_SET);
    write(fd, &inode, sizeof(struct i_node));
}


/************ Function to copy from v6 File System into an external File*******************/
void cpout(char *command, int d_start)
{
    char *part[3];
    int i, b1, b;
    splitCommand(part, command, " ");
    char *filepath[100];
    int lengthOfPath = splitCommand(filepath, part[1], "/");

    searchFileInPath(d_start, lengthOfPath, filepath);

    if (dir.inode_num == 0)
        return;
    int externalFileDes = open(part[2], O_WRONLY | O_CREAT,
                               S_IREAD | S_IEXEC | S_IWRITE);
    if (externalFileDes < 0)
    {
        perror("Unable to create an external file!");
    }

    long fileSize = getSize();
    int nFileBlocks = fileSize >> 9;
    int offset = fileSize & (512 - 1);
    if (offset > 0)
        nFileBlocks += 1;

    char buffer[512];
    if ((inode.flags >> 12) & 1)
    {
        int index = 0;
        unsigned short indBlockNo,block_no;
        //large file
        for (i = 0; i < 8; i++)
        {
            if (inode.addr[i] == 0)
                break;
            if (i == 7)
            {
                for (b1 = 0; b1 < 256; b1++)
                {
                    lseek(fd, (inode.addr[i] * 512)+(b1*sizeof(unsigned short)), SEEK_SET);
                    read(fd, &indBlockNo, sizeof(unsigned short));
                    for (b = 0; b < 256; b++)
                    {
                        if (index >= nFileBlocks)
                            return;

                        lseek(fd, (indBlockNo * 512)+(b*sizeof(unsigned short)), SEEK_SET);
                        read(fd, &block_no, sizeof(unsigned short));
                        lseek(fd, block_no * 512, SEEK_SET);

                        //To read the incomplete block
                        if (index == nFileBlocks - 1 && offset > 0)
                        {
                            read(fd, &buffer, offset);
                            write(externalFileDes, &buffer, offset);
                        }
                        else
                        {
                            read(fd, &buffer, sizeof(buffer));
                            write(externalFileDes, &buffer, sizeof(buffer));
                        }
                        index++;
                    }
                }
            }
            else
            {
                for (b = 0; b < 256; b++)
                {
                    if (index >= nFileBlocks)
                        return;
                    lseek(fd, (inode.addr[i] * 512) + (b * 2), SEEK_SET);
                    read(fd, &block_no, sizeof(unsigned short));
                    lseek(fd, block_no * 512, SEEK_SET);
                    if (index == nFileBlocks - 1 && offset > 0)
                    {
                        read(fd, &buffer, offset);
                        write(externalFileDes, &buffer, offset);
                    }
                    else
                    {
                        read(fd, &buffer, sizeof(buffer));
                        write(externalFileDes, &buffer, sizeof(buffer));
                    }
                    index++;
                }
            }
        }

    }
    //Small file
    else
    {
        for (i = 0; i < 8; i++)
        {
            if (inode.addr[i] == 0)
                break;
            lseek(fd, inode.addr[i] * 512, SEEK_SET);

            if (i == nFileBlocks - 1 && offset > 0)
            {
                read(fd, &buffer, offset);
                write(externalFileDes, &buffer, offset);
            }
            else
            {
                read(fd, &buffer, sizeof(buffer));
                write(externalFileDes, &buffer, sizeof(buffer));
            }
        }
    }
}


/********* Method for iterating throught the given path for cpout **********/
int searchFileInPath(int d_start, int lengthOfPath, char* filepath[100])
{
    int i, level, inode_i;

    lseek(fd, 1024, SEEK_SET);
    read(fd, &(inode), sizeof(struct i_node));                    //Read the inode of the root directory

    for (level = 0; level < lengthOfPath - 1; level++)
    {
        lseek(fd, d_start * 512, SEEK_SET);
        for (i = 0; i < 32; i++)
        {
            read(fd, &dir, sizeof(struct dir_entry));
            if (dir.inode_num != 0
                    && strcmp(dir.filename, filepath[level]) == 0)
            {
                lseek(fd, 1024 + ((dir.inode_num - 1) * 32), SEEK_SET);
                read(fd, &(inode), sizeof(struct i_node));
                d_start = inode.addr[0];
                break;
            }
        }
        if (i == 32)
        {
            printf("Given path does not exists...!\n");
            dir.inode_num = 0;
            return 0;
        }
    }

    for(inode_i=0; inode_i<8; inode_i++)
    {
        if(inode.addr[inode_i]==0)
            goto OUTSIDE;

        lseek(fd, inode.addr[inode_i] * 512, SEEK_SET);
        for (i = 0; i < 32; i++)
        {
            read(fd, &dir, sizeof(struct dir_entry));
            if (dir.inode_num != 0 && strcmp(dir.filename, filepath[level]) == 0)
                goto OUTSIDE;
        }
    }

OUTSIDE:
    if (i == 32)
    {
        printf("File not found in the given directory...!\n");
        dir.inode_num = 0;
        return 0;
    }

    lseek(fd, 1024 + ((dir.inode_num - 1) * 32), SEEK_SET);
    read(fd, &(inode), sizeof(struct i_node));
    return d_start;
}


/******* Method to create directory in v6 filesystem ********/
void mkdirectory(char* command, int d_start, int n_inodes)
{
    char *part[2];
    splitCommand(part, command, " ");
    char *filepath[100];
    int i,lengthOfPath = splitCommand(filepath, part[1], "/");

    int freeInode = getFreeInode(n_inodes);

    d_start = iteratePath(d_start,lengthOfPath,filepath);
    int parentInode = dir.inode_num;

    if(d_start==0)
        return;
    lseek(fd, d_start * 512, SEEK_SET);

    for (i = 0; i < 32; i++)
    {
        read(fd, &dir, sizeof(struct dir_entry));
        if (dir.inode_num!=0&&strcmp(dir.filename,filepath[lengthOfPath-1]) == 0)
        {
            printf("Directory already Exists..!\n");
            return;
        }

    }

    inode.flags = 0xcdff;
    inode.nlinks = 1;
    inode.uid = 0;
    inode.gid = 0;
    memset(inode.addr, 0, sizeof(inode.addr));
    inode.addr[0] = readFreeBlock();
    if(inode.addr[0]==0)
        return;
    populateSize(512);

    lseek(fd, d_start * 512, SEEK_SET);
    for (i = 0; i < 32; i++)
    {
        read(fd, &dir, sizeof(struct dir_entry));
        if (dir.inode_num == 0)
            break;
    }
    dir.inode_num = freeInode;
    strcpy(dir.filename, filepath[lengthOfPath-1]);
    lseek(fd, -16, SEEK_CUR);
    write(fd, &dir, sizeof(struct dir_entry));

    //writing i-node for directory
    lseek(fd, 1024 + ((dir.inode_num-1) * 32), SEEK_SET);
    write(fd, &inode, sizeof(struct i_node));

    dir.inode_num = freeInode;
    strcpy(dir.filename, ".\0");
    lseek(fd, inode.addr[0] * 512, SEEK_SET);
    write(fd, &dir, sizeof(struct dir_entry));

    dir.inode_num = parentInode;
    strcpy(dir.filename, "..\0");
    write(fd, &dir, sizeof(struct dir_entry));

    dir.inode_num = 0;
    strcpy(dir.filename, "\0");
    for (i = 0; i < 14; i++)
        write(fd, &dir, sizeof(struct dir_entry));
}


/******** Method to remove file from v6 file system ********/
void removev6file(char* command, int d_start)
{
    char *part[2];
    splitCommand(part, command, " ");
    char *filepath[100];
    int lengthOfPath = splitCommand(filepath, part[1], "/");
    d_start=searchFileInPath(d_start, lengthOfPath, filepath);
    if(d_start==0)
        return;

    removefileGenericMethod(filepath[lengthOfPath-1],d_start);
}


/******** Common method for removing both completed and uncompleted files ********/
void removefileGenericMethod(char* filename, int d_start)
{

    int i, b1, b;

//Add the data blocks to free array
    if ((inode.flags >> 12) & 1)
    {
        long fileSize = getSize();
        int nFileBlocks = fileSize >> 9;
        int offset = fileSize & (512 - 1);
        if (offset > 0)
            nFileBlocks = nFileBlocks + 1;
        int index = 0;
        unsigned short block_no, in_block_no;
        //large file
        for (i = 0; i < 8; i++)
        {
            if (inode.addr[i] == 0)
                break;
            lseek(fd, inode.addr[i] * 512, SEEK_SET);
            if (i == 7)
            {
                for (b1 = 0; b1 < 256; b1++)
                {
                    lseek(fd, (inode.addr[i] * 512)+(b1*sizeof(unsigned short)), SEEK_SET);
                    read(fd, &in_block_no, sizeof(unsigned short));
                    lseek(fd, in_block_no * 512, SEEK_SET);
                    for (b = 0; b < 256; b++)
                    {
                        if (index >= nFileBlocks)
                        {
                            addFreeBlock(in_block_no);
                            goto label;
                        }
                        read(fd, &block_no, sizeof(unsigned short));
                        addFreeBlock(block_no);
                        index++;
                    }
                    addFreeBlock(in_block_no);
                }
            }
            else
            {
                for (b = 0; b < 256; b++)
                {
                    if (index >= nFileBlocks)
                        goto label;
                    read(fd, &block_no, sizeof(unsigned short));
                    addFreeBlock(block_no);
                    index++;
                }
            }
        }
label:
        addFreeBlock(inode.addr[i]);
    }
    //Small file
    else
    {
        for (i = 0; i < 8; i++)
        {
            if (inode.addr[i] == 0)
                break;
            addFreeBlock(inode.addr[i]);
        }
    }

    //Free the inode
    resetinode();

    lseek(fd, 1024 + ((dir.inode_num - 1) * 32), SEEK_SET);
    write(fd, &inode, sizeof(struct i_node));

    //remove the file from the directory
    lseek(fd, d_start * 512, SEEK_SET);
    for (i = 0; i < 32; i++)
    {
        read(fd, &dir, sizeof(struct dir_entry));
        if (dir.inode_num!=0&&strcmp(dir.filename, filename) == 0)
            break;
    }
    dir.inode_num = 0;
    lseek(fd, -16, SEEK_CUR);
    write(fd, &dir, sizeof(struct dir_entry));
}


/******** Method for iterating through the given path *********/
int iteratePath(int d_start,int lengthOfPath,char* filepath[100])
{
    int level,i;
    for (level = 0; level < lengthOfPath - 1; level++)
    {
        lseek(fd, d_start * 512, SEEK_SET);
        for (i = 0; i < 32; i++)
        {
            read(fd, &dir, sizeof(struct dir_entry));
            if (dir.inode_num!=0&&strcmp(dir.filename, filepath[level]) == 0)
            {
                lseek(fd, 1024 + ((dir.inode_num - 1) * 32), SEEK_SET);
                read(fd, &(inode), sizeof(struct i_node));
                d_start = inode.addr[0];
                break;
            }
        }
        if (i == 32)
        {
            printf("Given path does not exists..!\n");
            return 0;
        }
    }
    return d_start;
}


/*****************reset inode********************/
void resetinode()
{
    inode.flags = 0x0dff;
    inode.nlinks = 0;
    inode.size0 = 0;
    inode.size1 = 0;
    memset(inode.addr, 0, sizeof(inode.addr));
}

