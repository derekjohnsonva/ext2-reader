#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cmath>

#include "ext2_fs.h"

#define BYTES_PRE_SUPER_BLOCK 1024
u_int32_t block_size;

bool check_istream_state(std::istream *fh)
{
    if (!*fh)
    {
        // std::cout << "error: only " << fh.gcount() << " could be read" << std::endl;
        std::cout << "eof "  << fh->eof()  << std::endl;
        std::cout << "fail " << fh->fail() << std::endl;
        std::cout << "bad "  << fh->bad()  << std::endl;
        return true;
    }
    return false;
}
 
// Function to convert octal number to decimal
int octal_to_decimal(int octalNumber)
{
    int decimalNumber = 0, i = 0, rem;
    while (octalNumber != 0)
    {
        rem = octalNumber % 10;
        octalNumber /= 10;
        decimalNumber += rem * std::pow(8, i);
        ++i;
    }
    return decimalNumber;
}


std::string convert_unix_epoch_to_date(time_t unix_epoch)
{
    struct tm *timeinfo;
    char buffer[80];
 
    timeinfo = localtime(&unix_epoch);
 
    strftime(buffer, 80, "%D %T", timeinfo);
    return std::string(buffer);
}

void print_superblock(std::ofstream& outfile, ext2_super_block sb)
{
    outfile << "SUPERBLOCK," <<
                sb.s_blocks_count << "," <<
                sb.s_inodes_count << "," <<
                block_size << "," <<
                sb.s_inode_size << "," <<
                sb.s_blocks_per_group << "," <<
                sb.s_inodes_per_group << "," <<
                sb.s_first_ino << std::endl;
}

int main()
{
    std::fstream fh;
    fh.open("test_data/trivial.img", std::ios::in | std::ios::binary);
    if (!fh.is_open())
    {
        std::cerr << "Could not open file" << std::endl;
        return 1;
    }
    // An Ext2 file systems starts with a superblock located at byte offset 1024 from the start of the volume.
    ext2_super_block sb;
    fh.seekg(1024);                                     // move 1024 bytes from the start of the file
    fh.read((char *)&sb, sizeof(sb)); // read the data into the superblock
    check_istream_state(&fh);
    // We will write the contents of the superblock to a .csv file
    std::ofstream outfile;
    outfile.open("output.csv");
    block_size = 1024 << sb.s_log_block_size;
    // outfile << "SUPERBLOCK," <<
    //             sb.s_blocks_count << "," <<
    //             sb.s_inodes_count << "," <<
    //             block_size << "," <<
    //             sb.s_inode_size << "," <<
    //             sb.s_blocks_per_group << "," <<
    //             sb.s_inodes_per_group << "," <<
    //             sb.s_first_ino << std::endl;
    print_superblock(outfile, sb);

    // Depending on how many block groups are defined, the Block Group Descriptor
    // table can require multiple blocks of storage.
    int block_group_count = (sb.s_blocks_count + sb.s_blocks_per_group - 1) / sb.s_blocks_per_group;
    // int blocks_for_block_group_descriptor = (sizeof(ext2_group_desc) * block_group_count) / block_size;
    std::vector<ext2_group_desc> block_group_descriptors;
    for (int i = 0; i < block_group_count; i++)
    {
        int pos = BYTES_PRE_SUPER_BLOCK + sizeof(sb) + (i * sizeof(ext2_group_desc));
        fh.seekg(pos, std::ios::beg); // move 2048 bytes from the start of the file
        if (check_istream_state(&fh)) {
            printf("error: could not seek to block group descriptor table\n");
            goto out1;
        }
        ext2_group_desc bgd;
        fh.read((char *)&bgd, sizeof(bgd));
        if (check_istream_state(&fh)) {
            printf("error: could not read data into block group\n");
            goto out1;
        }
        int32_t blocks_in_group = sb.s_blocks_count - (sb.s_blocks_per_group  * i);
        if (blocks_in_group < 0) {
            printf("error: blocks_in_group < 0\n");
            goto out1;
        }
        int32_t inodes_in_group = sb.s_inodes_count - (sb.s_inodes_per_group  * i);
        if (inodes_in_group < 0) {
            printf("error: inodes_in_group < 0\n");
            goto out1;
        }
        outfile << "GROUP," << 
            i << "," <<                                           // group number
            blocks_in_group << "," <<                             // total number of blocks in this group
            inodes_in_group << "," <<                             // total number of i-nodes in this group
            bgd.bg_free_blocks_count << "," << // number of free blocks
            bgd.bg_free_inodes_count << "," << // number of free i-nodes
            bgd.bg_block_bitmap << "," <<      // block number of free block bitmap for this group
            bgd.bg_inode_bitmap << "," <<      // block number of free i-node bitmap for this group
            bgd.bg_inode_table << std::endl;   // block number of first block of i-nodes in this group

        // iterate over each block in the block group
        for (int32_t i = 0; i < blocks_in_group; i++)
        {
            // read the block bitmap
            int pos = bgd.bg_block_bitmap * block_size + (i / 8);
            fh.seekg(pos, std::ios::beg);
            if (check_istream_state(&fh)) {
                printf("error: could not seek to block bitmap\n");
                goto out1;
            }
            char block_bitmap_byte;
            fh.read(&block_bitmap_byte, 1);
            if (check_istream_state(&fh)) {
                printf("error: could not read data into block bitmap\n");
                goto out1;
            }
            // check if the bit is set
            if ((block_bitmap_byte & (1 << (i % 8))) == 0)
            {
                outfile << "BFREE," << (i + 1) << std::endl; // TODO: Determine if this `+1` is correct
            }
        }
        // iterate over each inode in the block group
        for (int32_t i = 0; i < inodes_in_group; i++)
        {
            // read the block bitmap
            int pos = bgd.bg_inode_bitmap * block_size + (i / 8);
            fh.seekg(pos, std::ios::beg);
            if (check_istream_state(&fh)) {
                printf("error: could not seek to inode bitmap\n");
                goto out1;
            }
            char inode_bitmap_byte;
            fh.read(&inode_bitmap_byte, 1);
            if (check_istream_state(&fh)) {
                printf("error: could not read data into inode bitmap\n");
                goto out1;
            }
            // check if the bit is set
            if ((inode_bitmap_byte & (1 << (i % 8))) == 0)
            {
                outfile << "IFREE," << (i + 1) << std::endl; // TODO: Determine if this `+1` is correct
            }
        }

        // READ the INODE TABLE
        for (int32_t i = 0; i < inodes_in_group; i++) {
            // go to the location of the inode table
            int pos = bgd.bg_inode_table * block_size + (i * sizeof(ext2_inode));
            fh.seekg(pos, std::ios::beg);
            if (check_istream_state(&fh)) {
                printf("error: could not seek to inode table\n");
                goto out1;
            }
            ext2_inode inode_table;
            fh.read((char *)&inode_table, sizeof(inode_table));
            if (check_istream_state(&fh)) {
                printf("error: could not read data into inode table %i \n",i);
                goto out1;
            }
            char file_type = '?';
            if (inode_table.i_mode & EXT2_S_IFDIR) {
                file_type = 'd';
            } else if (inode_table.i_mode & EXT2_S_IFREG) {
                file_type = 'f';
            } else if (inode_table.i_mode & EXT2_S_IFLNK) {
                file_type = 's';
            }
            if (inode_table.i_mode !=0 && inode_table.i_links_count != 0) {
                outfile << "INODE," <<
                    (i + 1) << "," << // inode number (decimal)
                    file_type << "," <<  // file type ('f' for file, 'd' for directory, 's' for symbolic link, '?" for anything else)
                    inode_table.i_mode << "," << // TODO: mode (low order 12-bits, octal ... suggested format "%o")
                    inode_table.i_uid << "," << // owner (decimal)
                    inode_table.i_gid << "," << // group (decimal)
                    inode_table.i_links_count << "," << // link count (decimal)
                    //TODO: Wording is confusing for what is expected in below field
                    convert_unix_epoch_to_date(inode_table.i_ctime) << "," << // time of last I-node change (mm/dd/yy hh:mm:ss, GMT)
                    convert_unix_epoch_to_date(inode_table.i_mtime) << "," <<// modification time (mm/dd/yy hh:mm:ss, GMT)
                    convert_unix_epoch_to_date(inode_table.i_dtime) << "," <<// time of last access (mm/dd/yy hh:mm:ss, GMT)
                    inode_table.i_size << "," <<// file size (decimal)
                    inode_table.i_blocks;// number of (512 byte) blocks of disk space (decimal) taken up by this file
            
                /* For ordinary files (type 'f') and directories (type 'd') the next fifteen fields
                are block addresses (decimal, 12 direct, one indirect, one double indirect, 
                one triple indirect). */
                if (file_type == 'f' || file_type == 'd') {
                    for (int i = 0; i < 15; i++) {
                        outfile << "," << inode_table.i_block[i];
                    }
                }
                /* 
                Symbolic links. If the file length is less than the size of the
                block pointers (60 bytes) the file will contain zero data blocks,
                and the name (a text string) is stored in the space normally occupied
                by the block pointers. This is called an inline symbolic link.
                2 Cases:
                1) Inline: in field 13, print the integer that represents the name of the symbolic link
                2) Non-Inline: In filed 13+, only print non zero blocks
                */
                if (file_type == 's') {
                    if (inode_table.i_size < 60) {
                        outfile << "," << inode_table.i_block[0];
                    } else {
                        for (int i = 0; i < 15; i++) {
                            if (inode_table.i_block[i] != 0) {
                                outfile << "," << inode_table.i_block[i];
                            }
                        }
                    }
                }
                outfile << std::endl;
            }


        }


        block_group_descriptors.push_back(bgd);
    }

out1: 
    outfile.close();
    return 0;
}