#include <iostream>
#include <fstream>
#include <string> 
#include <vector>

#include "ext2_fs.h"

void check_istream_state(std::istream fh) {
    
}

int main() {
    std::fstream fh;
    fh.open("trivial.img", std::ios::in | std::ios::binary);
    if (!fh.is_open()) {
        std::cerr << "Could not open file" << std::endl;
        return 1;
    }
    // An Ext2 file systems starts with a superblock located at byte offset 1024 from the start of the volume.
    ext2_super_block super_block;
    fh.seekg(1024); // move 1024 bytes from the start of the file
    fh.read((char *)&super_block, sizeof(super_block)); // read the data into the superblock
    if (fh){
        std::cout << "All Characters read successfully." << std::endl;
    }
    else {
        std::cout << "error: only " << fh.gcount() << " could be read";
    }
    fh.close();

    // We will write the contents of the superblock to a .csv file
    std::ofstream outfile;
    outfile.open("output.csv");
    int block_size = 1024 << super_block.s_log_block_size;
    outfile << "SUPERBLOCK," << 
                super_block.s_blocks_count      << "," <<
                super_block.s_inodes_count      << "," <<
                block_size                      << "," <<
                super_block.s_inode_size        << "," <<
                super_block.s_blocks_per_group  << "," <<
                super_block.s_inodes_per_group  << "," <<
                super_block.s_first_ino         << std::endl;
    
    // Depending on how many block groups are defined, the Block Group Descriptor
    // table can require multiple blocks of storage.
    int block_group_count = (super_block.s_blocks_count + super_block.s_blocks_per_group - 1) / super_block.s_blocks_per_group;
    // int blocks_for_block_group_descriptor = (sizeof(ext2_group_desc) * block_group_count) / block_size;
    std::vector<ext2_group_desc> block_group_descriptors;
    for (int i=0; i < block_group_count; i++){
        fh.seekg(2048, std::ios::beg); // move 2048 bytes from the start of the file
        ext2_group_desc block_group_descriptor;
        fh.read((char *)&block_group_descriptor, sizeof(block_group_descriptor));
        if (!fh){
            // std::cout << "error: only " << fh.gcount() << " could be read" << std::endl;
            std::cout << "eof " << fh.eof() << std::endl;
            std::cout << "fail " << fh.fail() << std::endl;
            std::cout << "bad " << fh.bad() << std::endl;
        }
        printf("%d\n", block_group_descriptor.bg_block_bitmap);
        outfile << "GROUP," << 
                    i << "," << // group number
                    1 << "," << // #TODO: total number of blocks in this group
                    2 << "," << // #TODO: total number of i-nodes in this group
                    block_group_descriptor.bg_free_blocks_count << "," << // number of free blocks
                    block_group_descriptor.bg_free_inodes_count << "," << // number of free i-nodes
                    block_group_descriptor.bg_block_bitmap << "," << // block number of free block bitmap for this group
                    block_group_descriptor.bg_inode_bitmap<< "," << // block number of free i-node bitmap for this group
                    1 << // #TODO: block number of first block of i-nodes in this group
                    std::endl;

        block_group_descriptors.push_back(block_group_descriptor);
    }
    // int curr_block_count = 0;
    // for (auto it = block_group_descriptors.begin(); it != block_group_descriptors.end(); ++it){
    //     outfile << "GROUP," << 
    //                 curr_block_count << "," << // group number
    //                 1 << "," << // #TODO: total number of blocks in this group
    //                 2 << "," << // #TODO: total number of i-nodes in this group
    //                 it->bg_free_blocks_count << "," << // number of free blocks
    //                 it->bg_free_inodes_count << "," << // number of free i-nodes
    //                 it->bg_block_bitmap << "," << // block number of free block bitmap for this group
    //                 it->bg_inode_bitmap<< "," << // block number of free i-node bitmap for this group
    //                 1 << // #TODO: block number of first block of i-nodes in this group
    //                 std::endl;
    //     curr_block_count++;
    // }
    outfile.close();
}