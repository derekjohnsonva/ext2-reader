#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include "ext2_fs.h"

#define BYTES_PRE_SUPER_BLOCK 1024

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
    int block_size = 1024 << sb.s_log_block_size;
    outfile << "SUPERBLOCK," <<
                sb.s_blocks_count << "," <<
                sb.s_inodes_count << "," <<
                block_size << "," <<
                sb.s_inode_size << "," <<
                sb.s_blocks_per_group << "," <<
                sb.s_inodes_per_group << "," <<
                sb.s_first_ino << std::endl;

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

        block_group_descriptors.push_back(bgd);
    }

out1: 
    outfile.close();
    return 0;
}