# EXT2 Reader

Will take in a disk image formatted with the [EXT2 filesystem](https://en.wikipedia.org/wiki/Ext2).
The output will be a `.csv` file with info about the `.img` file.

## To Build
to build the executable `p4exp1`, run make in the project's root. Uses the `g++` [compiler](https://gcc.gnu.org/)

## To Clean
run `make clean` command from the project's root.

## Summary of Each File
The structure of this project is very simple.
* `main.cpp`: contains all of the code for parsing a EXT2 file system image. 
The `main` method accepts one command line argumen, the name of the file to parse.
It then calls the `read_ext2_image` method which performs all of the functionality.
* `ext2_fs.h`: contains the definitions for the EXT2 data structures (ex. `ext2_inode`). 
* `Makefile`: A very simple makefile with one target, `p4exp1`.
* `test.sh`: A script to validate the program.
* `testing_data`: a folder that contains `trivial.csv` and `trivial.img`. These two files can be used for testing

## TESTING
I did not perform any unit testing. In the future I would like to add unit tests.
TODO: Add tests using the [Catch2](https://github.com/catchorg/Catch2/blob/devel/docs/tutorial.md#top)
testing library.

All testing was integration testing done with the `test.sh` script.
To use this script, run it with two arguments. The first is the EXT2 image you want to dump
The second is a `.csv` file with the output you expect from the EXT2 reader.
The script will...
1) build the executable
2) Run the executable on the first argument (a `.img` file)
3) Compare the output of running the executable to the second argument (expected output). 
The comparison is done after a call to sort on both files being compared.

## Functionality

### superblock summary
SUPERBLOCK
1) total number of blocks (decimal)
1) total number of i-nodes (decimal)
1) block size (in bytes, decimal)
1) i-node size (in bytes, decimal)
1) blocks per group (decimal)
1) i-nodes per group (decimal)
1) first non-reserved i-node (decimal)

### group summary

1) GROUP
1) group number (decimal, starting from zero)
1) total number of blocks in this group (decimal)
1) total number of i-nodes in this group (decimal)
1) number of free blocks (decimal)
1) number of free i-nodes (decimal)
1) block number of free block bitmap for this group (decimal)
1) block number of free i-node bitmap for this group (decimal)
1) block number of first block of i-nodes in this group (decimal)

### free block entries

1) BFREE
1) number of the free block (decimal)

### free I-node entries

1) IFREE
1) number of the free I-node (decimal)

## I-node summary

1) INODE
1) inode number (decimal)
1) file type ('f' for file, 'd' for directory, 's' for symbolic link, '?" for anything else)
1) mode (low order 12-bits, octal ... suggested format "%o")
1) owner (decimal)
1) group (decimal)
1) link count (decimal)
1) time of last I-node change (mm/dd/yy hh:mm:ss, GMT)
1) modification time (mm/dd/yy hh:mm:ss, GMT)
1) time of last access (mm/dd/yy hh:mm:ss, GMT)
1) file size (decimal)
1) number of (512 byte) blocks of disk space (decimal) taken up by this file

### Symbolic links. 

For an inline symbolic link:

13)  print the first four bytes of the name as unsigned int (decimal)


### directory entries

1) DIRENT
1) parent inode number (decimal) ... the I-node number of the directory that contains this entry
1) logical byte offset (decimal) of this entry within the directory
1) inode number of the referenced file (decimal)
1) entry length (decimal)
1) name length (decimal)
1) name (string, surrounded by single-quotes). Don't worry about escaping, we promise there will be no single-quotes or commas in any of the file names.

### indirect block references

1) INDIRECT
1) I-node number of the owning file (decimal)
1) (decimal) level of indirection for the block being scanned ... 1 for single indirect, 2 for double indirect, 3 for triple
1) logical block offset (decimal) represented by the referenced block. If the referenced block is a data block, this is the logical block offset of that block within the file. If the referenced block is a single- or double-indirect block, this is the same as the logical offset of the first data block to which it refers.
1) block number of the (1, 2, 3) indirect block being scanned (decimal) . . . not the highest level block (in the recursive scan), but the lower level block that contains the block reference reported by this entry.
1) block number of the referenced block (decimal)
