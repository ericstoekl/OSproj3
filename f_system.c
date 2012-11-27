/* Erich Stoekl, Penn State University, 2012
 * ems5311@psu.edu
 *
 * f_system function implementations
 *
 */
#include <string.h>
#include <stdio.h>

#include "f_system.h"
#include "storage.h"

#define DIR_NAME_MAX 27

extern unsigned int *filesys;

// Input: name of the directory that will be created
// Result: a single block is allocated as a directory.
// Returned: block address that is now in use.
int create_struct_dir(const char *dir_name)
{
    // (32 bytes) are used per 'line' of the struct dir.
    // (4 bytes) of the first line must be left to be used
    // by the 'pointer' which can point to the continuation of this struct dir.
    // Therefore the dir_name can be 27 bytes (28 characters with null term) max.

    unsigned int name_len = strlen(dir_name);
    if(name_len > DIR_NAME_MAX)
    {
        fprintf(stderr, "%s: directory name too large (>27)\n", __func__);
        return -1;
    }

    free_blks_bounds blk_bnds;
    blk_bnds = find_free_blocks(1);
    int blk_index = blk_bnds.start; // This is the first free block we found.
    flag_bit(blk_index);

    // Put the name of this directory in the first 28 bytes (7 ints) of the block we just found:
    memcpy((char *)(filesys + (blk_index * BLK_SZ_INT)), dir_name, name_len+1);

    printf("'%s', block %d\n", (char *)(filesys + (blk_index * BLK_SZ_INT)), blk_index);

}
