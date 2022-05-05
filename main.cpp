#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cmath>
#include <optional>

#include "ext2_fs.h"

#define BYTES_PRE_SUPER_BLOCK 1024
u_int block_size;
// The amount of bytes that can be stored in the direct block (i.e. first 12)
u_int size_of_direct_blocks;
// the amout of bytes that can be stored in the single indirect block (i.e. block 13)
u_int size_of_single_indirect;
// the amout of bytes that can be stored in the double indirect block (i.e. block 14)
u_int size_of_double_indirect;
// the amout of bytes that can be stored in the triple indirect block (i.e. block 15)
u_int size_of_triple_indirect;

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
 
// Function to convert decimal number to octal
int decimal_to_octal(int decimalNumber)
{
    int rem, i = 1, octalNumber = 0;
    while (decimalNumber != 0)
    {
        rem = decimalNumber % 8;
        decimalNumber /= 8;
        octalNumber += rem * i;
        i *= 10;
    }
    return octalNumber;
}


std::string convert_unix_epoch_to_date(time_t unix_epoch)
{
    struct tm *timeinfo;
    char buffer[80];
 
    timeinfo = gmtime(&unix_epoch);
 
    strftime(buffer, 80, "%D %T", timeinfo);
    return std::string(buffer);
}

void print_superblock(ext2_super_block sb)
{
    std::cout << "SUPERBLOCK," <<
                sb.s_blocks_count << "," <<
                sb.s_inodes_count << "," <<
                block_size << "," <<
                sb.s_inode_size << "," <<
                sb.s_blocks_per_group << "," <<
                sb.s_inodes_per_group << "," <<
                sb.s_first_ino << std::endl;
}

std::optional<__u32> get_indirect_block(uint ind_block, uint ind_offset, std::fstream& fh)
{
    int position = ind_block * block_size + ind_offset;
    fh.seekg(position, std::ios::beg);
    if (check_istream_state(&fh)) {
        printf("error: could not seek to indirect block\n");
        return {};
    }
    __u32 block_number;
    fh.read((char *)&block_number, sizeof(__u32));
    if (check_istream_state(&fh)) {
        printf("error: could not read indirect block\n");
        return {};
    }
    return block_number;

}

bool print_indirect_blocks(uint ind_block_num, int32_t inode, int logical_offset,
                           std::fstream& fh)
{
    // Read the indirect block
    uint curr_offset = 0;
    while (true) {
        auto block_number  = get_indirect_block(ind_block_num, curr_offset, fh);
        // handle the case where get indirect block returned nothing
        if (!block_number.has_value()) {
            printf("Failed on getting data block from single indirect block\n");
            return false;
        }
        if (*block_number != 0) {
            std::cout << "INDIRECT," <<
                    (inode + 1) << "," << // I-node number of the owning file (decimal)
                        1 << "," <<// (decimal) level of indirection for the block being scanned ... 1 for single indirect, 2 for double indirect, 3 for triple
                        logical_offset + curr_offset / sizeof(__u32) << "," <<// logical block offset (decimal) represented by the referenced block. If the referenced block is a data block, this is the logical block offset of that block within the file. If the referenced block is a single- or double-indirect block, this is the same as the logical offset of the first data block to which it refers.
                        ind_block_num << "," <<// block number of the (1, 2, 3) indirect block being scanned (decimal) . . . not the highest level block (in the recursive scan), but the lower level block that contains the block reference reported by this entry.
                        *block_number << std::endl; // block number of the referenced block (decimal)
        }
        curr_offset += sizeof(__u32);
        if (curr_offset >= block_size) { break; }
    }
    return true;
}

bool print_2nd_indirect_blocks(uint ind_block_num, int32_t inode, int logical_offset,
                               std::fstream& fh)
{
    // Read the indirect block
    uint curr_offset = 0;
    while (true) {
        auto block_number  = get_indirect_block(ind_block_num, curr_offset, fh);
        // handle the case where get indirect block returned nothing
        if (!block_number.has_value()) {
            printf("Failed on getting single indirect block from double indirect block\n");
            return false;
        }
        if (*block_number != 0) {
            std::cout << "INDIRECT," <<
                        (inode + 1) << "," << // I-node number of the owning file (decimal)
                        2 << "," <<// (decimal) level of indirection for the block being scanned ... 1 for single indirect, 2 for double indirect, 3 for triple
                        logical_offset + curr_offset / sizeof(__u32) << "," <<// logical block offset (decimal) represented by the referenced block. If the referenced block is a data block, this is the logical block offset of that block within the file. If the referenced block is a single- or double-indirect block, this is the same as the logical offset of the first data block to which it refers.
                        ind_block_num << "," <<// block number of the (1, 2, 3) indirect block being scanned (decimal) . . . not the highest level block (in the recursive scan), but the lower level block that contains the block reference reported by this entry.
                        *block_number << std::endl; // block number of the referenced block (decimal)
            print_indirect_blocks(*block_number, inode, logical_offset + (curr_offset / sizeof(__u32)), fh);
        }
        curr_offset += sizeof(__u32);
        if (curr_offset >= block_size) { break; }
    }
    return true;
}

bool print_3rd_indirect_blocks(uint ind_block_num, int32_t inode, int logical_offset,
                               std::fstream& fh)
{
    // Read the indirect block
    uint curr_offset = 0;
    while (true) {
        auto block_number  = get_indirect_block(ind_block_num, curr_offset, fh);
        // handle the case where get indirect block returned nothing
        if (!block_number.has_value()) {
            printf("Failed on getting double indirect block from triple indirect block\n");
            return false;
        }
        if (*block_number != 0) {
            std::cout << "INDIRECT," <<
                        (inode + 1) << "," << // I-node number of the owning file (decimal)
                        3 << "," <<// (decimal) level of indirection for the block being scanned ... 1 for single indirect, 2 for double indirect, 3 for triple
                        logical_offset + curr_offset / sizeof(__u32) << "," <<// logical block offset (decimal) represented by the referenced block. If the referenced block is a data block, this is the logical block offset of that block within the file. If the referenced block is a single- or double-indirect block, this is the same as the logical offset of the first data block to which it refers.
                        ind_block_num << "," <<// block number of the (1, 2, 3) indirect block being scanned (decimal) . . . not the highest level block (in the recursive scan), but the lower level block that contains the block reference reported by this entry.
                        *block_number << std::endl; // block number of the referenced block (decimal)
            print_2nd_indirect_blocks(*block_number, inode, logical_offset + (curr_offset / sizeof(__u32)), fh);
        }
        curr_offset += sizeof(__u32);
        if (curr_offset >= block_size) { break; }
    }
    return true;
}

bool get_all_indirect_blocks(uint ind_block_num, std::vector<__u32>& out_vec, std::fstream& fh)
{
    // Read the indirect block
    uint curr_offset = 0;
    while (curr_offset < block_size) {
        auto block_number  = get_indirect_block(ind_block_num, curr_offset, fh);
        // handle the case where get indirect block returned nothing
        if (!block_number.has_value()) {
            printf("Failed on getting data block from indirect block\n");
            return false;
        }
        if (*block_number != 0){
            out_vec.push_back(*block_number);
        }
        curr_offset += sizeof(__u32);
    }
    return true;
}

bool get_all_double_indirect_blocks(uint ind_block_num, std::vector<__u32>& out_vec, std::fstream& fh)
{
    // Read the indirect block
    uint curr_offset = 0;
    while (curr_offset < block_size) {
        auto block_number  = get_indirect_block(ind_block_num, curr_offset, fh);
        // handle the case where get indirect block returned nothing
        if (!block_number.has_value()) {
            printf("Failed on getting data block from double indirect block\n");
            return false;
        }
        if (*block_number != 0){
            get_all_indirect_blocks(*block_number, out_vec, fh);
        }
        curr_offset += sizeof(__u32);
    }
    return true;
}

bool get_all_triple_indirect_blocks(uint ind_block_num, std::vector<__u32>& out_vec, std::fstream& fh)
{
    // Read the indirect block
    uint curr_offset = 0;
    while (curr_offset < block_size) {
        auto block_number  = get_indirect_block(ind_block_num, curr_offset, fh);
        // handle the case where get indirect block returned nothing
        if (!block_number.has_value()) {
            printf("Failed on getting data block from double indirect block\n");
            return false;
        }
        if (*block_number != 0){
            get_all_double_indirect_blocks(*block_number, out_vec, fh);
        }
        curr_offset += sizeof(__u32);
    }
    return true;
}

bool print_directory_entries(ext2_inode inode_table, int inode_num, std::fstream& fh)
{
    // READ the DIRECTORY ENTRIES
    // For each directory I-node, scan every data block.
    // For each non-zero data block, print directory information
    /* We will start by getting all of the data blocks*/
    std::vector<__u32> data_blocks;
    for (int i=0; i < EXT2_NDIR_BLOCKS; i++) {
        data_blocks.push_back(inode_table.i_block[i]);
    }
    if (inode_table.i_block[EXT2_IND_BLOCK] != 0){
        if (!get_all_indirect_blocks(inode_table.i_block[EXT2_IND_BLOCK], data_blocks, fh)) return false;
    }
    if (inode_table.i_block[EXT2_DIND_BLOCK] != 0){
        if (!get_all_indirect_blocks(inode_table.i_block[EXT2_DIND_BLOCK], data_blocks, fh)) return false;
    }
    if (inode_table.i_block[EXT2_TIND_BLOCK] != 0){
        if (!get_all_indirect_blocks(inode_table.i_block[EXT2_TIND_BLOCK], data_blocks, fh)) return false;
    }

    uint curr_offset = 0;
    while (curr_offset < inode_table.i_size)
    {
        // First, we will go to the first data block for the directory
        int block_list_index = curr_offset / block_size; 
        // The first 12 data blocks are direct blocks
        int start_pos = data_blocks.at(block_list_index) * block_size + (curr_offset % block_size);
        fh.seekg(start_pos, std::ios::beg);
        if (check_istream_state(&fh)) {
            printf("error: could not seek to directory data block\n");
            return false;
        }
        // Now, we will read the data block
        ext2_dir_entry dir_entry;
        fh.read((char *)&dir_entry, sizeof(ext2_dir_entry));
        if (check_istream_state(&fh)) {
            printf("error: could not read data into directory data block\n");
            return false;
        }
        if (dir_entry.inode != 0) {
            std::cout << "DIRENT," <<
                        (inode_num + 1) << "," << // parent inode number (decimal) ... the I-node number of the directory that contains this entry
                        curr_offset << "," << // logical byte offset (decimal) of this entry within the directory
                        dir_entry.inode << "," << // inode number of the referenced file (decimal)
                        dir_entry.rec_len << "," << // entry length (decimal)
                        // I am not sure why name_len has to be cast to an int to work.
                        (int) dir_entry.name_len << ","; // name length (decimal)
            // name (string, surrounded by single-quotes). Don't worry about escaping, we promise there will be no single-quotes or commas in any of the file names.
            std::cout << "'";
            for (int ii=0; ii < (int) dir_entry.name_len; ii++) {
                std::cout << dir_entry.name[ii];
            }
            std::cout << "'" << std::endl;
        }
        curr_offset += dir_entry.rec_len;
    }
    return true;
}

int read_ext2_image(const char *in_file) {
    std::fstream fh;
    fh.open(in_file, std::ios::in | std::ios::binary);
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
    block_size = 1024 << sb.s_log_block_size;
    size_of_direct_blocks = block_size * 12;
    int block_ids_stored_in_one_block = block_size / sizeof(__u32); // __u32 is the datatype that block numbers are stored in
    size_of_single_indirect = block_size * block_ids_stored_in_one_block;
    size_of_double_indirect = size_of_single_indirect * block_ids_stored_in_one_block;
    size_of_triple_indirect = size_of_double_indirect * block_ids_stored_in_one_block;
    print_superblock(sb);

    // Depending on how many block groups are defined, the Block Group Descriptor
    // table can require multiple blocks of storage.
    int block_group_count = (sb.s_blocks_count + sb.s_blocks_per_group - 1) / sb.s_blocks_per_group;
    // int blocks_for_block_group_descriptor = (sizeof(ext2_group_desc) * block_group_count) / block_size;
    for (int i = 0; i < block_group_count; i++)
    {
        int pos = BYTES_PRE_SUPER_BLOCK + sizeof(sb) + (i * sizeof(ext2_group_desc));
        fh.seekg(pos, std::ios::beg); // move 2048 bytes from the start of the file
        if (check_istream_state(&fh)) {
            printf("error: could not seek to block group descriptor table\n");
            return 1;
        }
        ext2_group_desc bgd;
        fh.read((char *)&bgd, sizeof(bgd));
        if (check_istream_state(&fh)) {
            printf("error: could not read data into block group\n");
            return 1;
        }
        int32_t blocks_in_group = sb.s_blocks_count - (sb.s_blocks_per_group  * i);
        if (blocks_in_group < 0) {
            printf("error: blocks_in_group < 0\n");
            return 1;
        }
        int32_t inodes_in_group = sb.s_inodes_count - (sb.s_inodes_per_group  * i);
        if (inodes_in_group < 0) {
            printf("error: inodes_in_group < 0\n");
            return 1;
        }
        std::cout << "GROUP," << 
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
                return 1;
            }
            char block_bitmap_byte;
            fh.read(&block_bitmap_byte, 1);
            if (check_istream_state(&fh)) {
                printf("error: could not read data into block bitmap\n");
                return 1;
            }
            // check if the bit is set
            if ((block_bitmap_byte & (1 << (i % 8))) == 0)
            {
                std::cout << "BFREE," << (i + 1) << std::endl; // TODO: Determine if this `+1` is correct
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
                return 1;
            }
            char inode_bitmap_byte;
            fh.read(&inode_bitmap_byte, 1);
            if (check_istream_state(&fh)) {
                printf("error: could not read data into inode bitmap\n");
                return 1;
            }
            // check if the bit is set
            if ((inode_bitmap_byte & (1 << (i % 8))) == 0)
            {
                std::cout << "IFREE," << (i + 1) << std::endl; // TODO: Determine if this `+1` is correct
            }
        }

        // READ the INODE TABLE
        for (int32_t i = 0; i < inodes_in_group; i++) {
            // go to the location of the inode table
            int pos = bgd.bg_inode_table * block_size + (i * sizeof(ext2_inode));
            fh.seekg(pos, std::ios::beg);
            if (check_istream_state(&fh)) {
                printf("error: could not seek to inode table\n");
                return 1;
            }
            ext2_inode inode_table;
            fh.read((char *)&inode_table, sizeof(inode_table));
            if (check_istream_state(&fh)) {
                printf("error: could not read data into inode table %i \n",i);
                return 1;
            }
            char file_type = '?';
            if ((inode_table.i_mode & EXT2_I_MODE_MASK) == EXT2_I_IFDIR) {file_type = 'd';}
            else if ((inode_table.i_mode & EXT2_I_MODE_MASK) == EXT2_I_IFREG) {file_type = 'f';} 
            else if ((inode_table.i_mode & EXT2_I_MODE_MASK) == EXT2_I_IFLNK) { file_type = 's';}
            if (inode_table.i_mode !=0 && inode_table.i_links_count != 0) {
                std::cout << "INODE," <<
                    (i + 1) << "," << // inode number (decimal)
                    file_type << "," <<  // file type ('f' for file, 'd' for directory, 's' for symbolic link, '?" for anything else)
                    decimal_to_octal(inode_table.i_mode & 0xFFF) << "," << // mode (low order 12-bits, octal ... suggested format "%o")
                    inode_table.i_uid << "," << // owner (decimal)
                    inode_table.i_gid << "," << // group (decimal)
                    inode_table.i_links_count << "," << // link count (decimal)
                    //TODO: Wording is confusing for what is expected in below field
                    convert_unix_epoch_to_date(inode_table.i_ctime) << "," << // time of last I-node change (mm/dd/yy hh:mm:ss, GMT)
                    convert_unix_epoch_to_date(inode_table.i_mtime) << "," << // modification time (mm/dd/yy hh:mm:ss, GMT)
                    convert_unix_epoch_to_date(inode_table.i_atime) << "," << // time of last access (mm/dd/yy hh:mm:ss, GMT)
                    inode_table.i_size << "," <<// file size (decimal)
                    inode_table.i_blocks;// number of (512 byte) blocks of disk space (decimal) taken up by this file
            
                /* For ordinary files (type 'f') and directories (type 'd') the next fifteen fields
                are block addresses (decimal, 12 direct, one indirect, one double indirect, 
                one triple indirect). */
                if (file_type == 'f' || file_type == 'd') {
                    for (int i = 0; i < 15; i++) {
                        std::cout << "," << inode_table.i_block[i];
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
                        std::cout << "," << inode_table.i_block[0];
                    } else {
                        for (int i = 0; i < 15; i++) {
                            if (inode_table.i_block[i] != 0) {
                                std::cout << "," << inode_table.i_block[i];
                            }
                        }
                    }
                }
                std::cout << std::endl;

                if (file_type == 'd') {
                    if (!print_directory_entries(inode_table, i, fh)) return 1;
                }

                // INDIRECT BLOCKS
                if (file_type == 'd' || file_type == 'f') {
                    // Initially, the logical offset will be equal to the number of data blocks
                    int logical_offset = EXT2_NDIR_BLOCKS;
                    if (inode_table.i_block[EXT2_IND_BLOCK] != 0) {
                        if (!print_indirect_blocks(inode_table.i_block[EXT2_IND_BLOCK], i, logical_offset, fh)) { return 1; }
                    }
                    // update logical offset to be equal the number of data blocks + the
                    // number of block referenced by indirect blocks
                    int blocks_referenced_by_indirect_block = block_size / sizeof(__u32);
                    logical_offset += blocks_referenced_by_indirect_block;
                    if (inode_table.i_block[EXT2_DIND_BLOCK] != 0) {
                        if (!print_2nd_indirect_blocks(inode_table.i_block[EXT2_DIND_BLOCK], i, logical_offset, fh)) { return 1; }
                    }
                    // update logical offset to be equal the number of data blocks + the
                    // number of block referenced by single indirect blocks + the number of
                    // blocks reference by double indirect blocks
                    logical_offset += std::pow(blocks_referenced_by_indirect_block, 2);
                    if (inode_table.i_block[EXT2_TIND_BLOCK] != 0) {
                        if (!print_3rd_indirect_blocks(inode_table.i_block[EXT2_TIND_BLOCK], i, logical_offset, fh)) { return 1; }
                    }

                }

            }


        }
    }
    return 0;
}

// main method should take one command line argument, 
// the path to the image file,

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("usage: %s <image file>\n", argv[0]);
        return 1;
    }
    // check to see that the first command line argument is a .img file
    std::string image_file_name = argv[1];
    if (image_file_name.substr(image_file_name.size() - 4) != ".img") {
        printf("error: %s is not a .img file\n", image_file_name.c_str());
        return 1;
    }
    return read_ext2_image(argv[1]);
}