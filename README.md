# unixv6filesystem
This project contains the C code for a model of Unix v6 File System.
 
//****************************************************************************************************************************************
//Unix V6 File System Model	
//****************************************************************************************************************************************

The project can be compiled and executed by any IDE such as CodeBlocks, Eclipse,..etc

This code supports the following commands.

1) fsaccess : For entering into the file system
              Example: fsaccess v6 // Here v6 denotes the name of the special file on which we are going 
                                      to mount our file system. 

2) initfs : Used for initializing the file system and specifying the limits on the number of data blocks and inodes.
            Example: 5000 400 // This command initializes the filesystem with 5000 data blocks and 400 inodes.

3) mkdir : This command can be used for creating a new directory in the given path
           Example: mkdir /a    //Creates a new directory named 'a' under the root directory
                    mkdir /a/b  //Creates a new directory under directory 'a'

4) cpin: Creates a new file in the given path and also copies the contents of the given external file into it.
         Example: cpin externalfilename /directoryname/internalfilename

5) cpout: Copies the content of the given file into an external file.
          Example: cpout /directoryname/name2.txt name3.txt
                   cpout /a/ex1.txt out1.txt

6) Rm : To remove a file or directory. 
        Example:Rm Filename.txt       
                Rm /a/ex1.txt        

7) q: To quit the terminal
 
