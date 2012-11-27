/* Erich Stoekl, Penn State University, 2012
 * ems5311@psu.edu
 *
 * f_system function implementations
 *
 */
#include <string.h>
#include <stdio.h>
#include <malloc.h>

#include "f_system.h"
#include "storage.h"

extern int debug;

extern unsigned int *filesys;

extern unsigned int cur_dir;

/*
typedef struct str_dir
{
    char dir_name[28]; // 28 bytes to hold the name of this directory
    int more_data;     // block address of block that holds more data (if necessary)
    int dir_contents[120]; // 120 ints (15 rows) to account for the rest of the data
                           // in this directory block.
} struct_dir;
*/

// Input: name of the directory that will be created
// Result: a single block is allocated as a directory.
// Returned: block address that is now in use.
int create_struct_dir(const char *dir_name)
{
    // (32 bytes) are used per 'line' of the struct dir.
    // (4 bytes) of the first line must be left to be used
    // by the 'pointer' which can point to the continuation of this struct dir.
    // Therefore the dir_name can be 27 bytes (28 characters with null term) max.
    struct_dir *directory;
    if(debug) printf("struct_dir is %d bytes.\n", (int)sizeof(struct_dir));
    directory = (struct_dir *)malloc(sizeof(struct_dir));

    unsigned int name_len = strlen(dir_name);
    if(name_len > DIR_NAME_MAX-1)
    {
        fprintf(stderr, "%s: directory name too large (>DIR_NAME_MAX)\n", __func__);
        return -1;
    }

    // Copy the bytes over, deep:
    int i;
    for(i = 0; dir_name[i] != '\0'; i++)
        directory->dir_name[i] = dir_name[i];
    directory->dir_name[i] = '\0';

    // All the dir_contents[] should be set to 0xDEADBEEF, to denote that they
    // are empty and ready to be filled.
    for(i = 0; i < 120; i++)
    {
        directory->dir_contents[i] = 0xDEADBEEF;
    }

    // Find the location in the filesystem where we can put this directory struct.

    free_blks_bounds blk_bnds;
    blk_bnds = find_free_blocks(1);
    int blk_index = blk_bnds.start; // This is the first free block we found.

    flag_bit(blk_index); // Make sure to flag the block we just used in the bitmap!

    // Deep copy everything from the directory struct over to the filesys at this blk_index:
    memcpy((struct_dir *)(filesys + (blk_index * BLK_SZ_INT)), directory, BLK_SZ_BYTE);

    free(directory);
    directory = NULL;

    if(debug) printf("'%s', block %d\n", (char *)(filesys + (blk_index * BLK_SZ_INT)), blk_index);
/*
    // Now I can easily store the int 'pointer' to the next struct dir or FCB like so:
    *((filesys + (blk_index * BLK_SZ_INT))+7) = 4000;
*/

// Now we have a new directory stored in the filesystem in a single block.
    return blk_index;
    
}
