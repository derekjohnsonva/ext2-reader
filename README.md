# EXT2 Reader

Will take in a disk image formatted with the [EXT2 filesystem](https://en.wikipedia.org/wiki/Ext2).
The output will be a `.csv` file with info about the `.img` file.

## superblock summary
SUPERBLOCK
1) total number of blocks (decimal)
1) total number of i-nodes (decimal)
1) block size (in bytes, decimal)
1) i-node size (in bytes, decimal)
1) blocks per group (decimal)
1) i-nodes per group (decimal)
1) first non-reserved i-node (decimal)

## group summary

1) GROUP
1) group number (decimal, starting from zero)
1) total number of blocks in this group (decimal)
1) total number of i-nodes in this group (decimal)
1) number of free blocks (decimal)
1) number of free i-nodes (decimal)
1) block number of free block bitmap for this group (decimal)
1) block number of free i-node bitmap for this group (decimal)
1) block number of first block of i-nodes in this group (decimal)

## free block entries

1) BFREE
1) number of the free block (decimal)

## free I-node entries

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


## directory entries

1) DIRENT
1) parent inode number (decimal) ... the I-node number of the directory that contains this entry
1) logical byte offset (decimal) of this entry within the directory
1) inode number of the referenced file (decimal)
1) entry length (decimal)
1) name length (decimal)
1) name (string, surrounded by single-quotes). Don't worry about escaping, we promise there will be no single-quotes or commas in any of the file names.

## indirect block references

1) INDIRECT
1) I-node number of the owning file (decimal)
1) (decimal) level of indirection for the block being scanned ... 1 for single indirect, 2 for double indirect, 3 for triple
1) logical block offset (decimal) represented by the referenced block. If the referenced block is a data block, this is the logical block offset of that block within the file. If the referenced block is a single- or double-indirect block, this is the same as the logical offset of the first data block to which it refers.
1) block number of the (1, 2, 3) indirect block being scanned (decimal) . . . not the highest level block (in the recursive scan), but the lower level block that contains the block reference reported by this entry.
1) block number of the referenced block (decimal)

## TESTING
TODO: Add tests using the [Catch2](https://github.com/catchorg/Catch2/blob/devel/docs/tutorial.md#top)
testing library.