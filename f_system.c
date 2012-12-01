/* Erich Stoekl, Joong Hun Kwak
 * ems5311@psu.edu, jwk5221@psu.edu
 *
 * f_system function implementations
 *
 */
#include <string.h>
#include <stdio.h>
#include <malloc.h>
#include <stdbool.h>

#include "f_system.h"
#include "storage.h"

extern int debug;

extern unsigned int *filesys;

extern unsigned int cur_dir;

/*
typedef struct str_dir
{
    char dir_name[28]; // 28 bytes to hold the name of this directory
    int filled_count;  // keeps track of how many entries are in the directory.
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
    // (4 bytes) will be used to keep track of how many entries are in this block.
    // (4 bytes) of the first line must be left to be used
    // by the 'pointer' which can point to the continuation of this struct dir.
    // Therefore the dir_name can be 23 bytes (24 characters with null term) max.
    struct_dir *directory;
    if(debug) printf("struct_dir is %d bytes.\n", (int)sizeof(struct_dir));
    directory = (struct_dir *)malloc(sizeof(struct_dir));

    unsigned int name_len = strlen(dir_name);
    if(name_len > DIR_NAME_MAX-1)
    {
        fprintf(stderr, "%s: directory name too large (>DIR_NAME_MAX)\n", __func__);
        return -1;
    }

    // Copy the dir_name bytes over, deep:
    int i;
    for(i = 0; dir_name[i] != '\0'; i++)
        directory->dir_name[i] = dir_name[i];
    directory->dir_name[i] = '\0';

    // Set the number of dir entries to 0:
    directory->filled_count = 0;

    // This is a new struct dir, so set more_data to deadbeef:
    directory->more_data = 0xDEADBEEF;

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

    // Set the first dir_contents to '.', or pointer to this directory:
    memmove((char *)(directory->dir_contents), ".", 2);
    directory->dir_contents[6] = IS_DIR;
    directory->dir_contents[7] = blk_index;
    directory->filled_count++;

    // If this isn't the root directory, create '..' directory which reference the parent dir:
    if(strcmp(dir_name, "root") != 0)
    {
        memmove((char *)(directory->dir_contents + 8), "..", 3);
        directory->dir_contents[14] = IS_DIR;
        directory->dir_contents[15] = cur_dir;
        directory->filled_count++;
    }

    // Deep copy everything from the directory struct over to the filesys at this blk_index:
    memcpy((struct_dir *)(filesys + (blk_index * BLK_SZ_INT)), directory, BLK_SZ_BYTE);

    free(directory);
    directory = NULL;

    if(debug) printf("'%s', block %d\n", (char *)(filesys + (blk_index * BLK_SZ_INT)), blk_index);

    // Now we have a new directory stored in the filesystem in a single block.
    return blk_index;
    
}

int add_entry(const char* name, const int _addr, const int type)
{
    int namelen = strlen(name);
    if(namelen > DIR_NAME_MAX-1)
    {
        fprintf(stderr, "%s: name too large (>DIR_NAME_MAX)\n", __func__);
        return -1;
    }
    if(_addr > 4095 || _addr < 0)
    {
        fprintf(stderr, "%s: _addr too large or too small.\n", __func__);
        return -1;
    }

    // 1: Find the first unused row of the current directory (cur_dir)

    // We can start searching at index 8 of this block:
    int blk_start = cur_dir * BLK_SZ_INT;
    int i;
    bool repeat;

    do
    {
        repeat = false;
        i = blk_start;
        for(i += 8; i < (blk_start + BLK_SZ_INT); i += 8)
        {
            // If the first int of the row is DEADBEEF, use this row:
            if(filesys[i] == 0xDEADBEEF)
                break;
        }
        if(filesys[blk_start + 7] != 0xDEADBEEF)
        {
            blk_start = filesys[blk_start + 7] * BLK_SZ_INT;
            repeat = true;
        }
    } while(repeat);

    // Check if the struct dir is full
    if(i == blk_start + BLK_SZ_INT)
    {
        // this struct_dir is full! we need to create a new one.
        int ext_index = create_struct_dir((char *)(filesys + cur_dir * BLK_SZ_INT));
        filesys[blk_start + 7] = ext_index;
    }

    // 2: copy the name into the first 7 ints (directory can have 23 characters max).
    memmove((char *)(filesys + i), name, namelen+1);

    // 3: copy the dir_addr into the final int (final 4 bytes).
    filesys[i + 7] = _addr;

    // also write into the row that this row is a directory entry:
    filesys[i + 6] = type;

    // 4: increment the counter keeping track of the number of entries in the dir:
    filesys[blk_start + FILLED_COUNT] ++;

    return 0;
}

// RETURNS an actual block index if 'name' successfully found.
int check_name(const char *name)
{
    int i;
    int blk_start = (cur_dir * BLK_SZ_INT);
    int filled_count = filesys[blk_start + FILLED_COUNT];
    int indexable_count = (filled_count+1)*8 + blk_start;
    bool is_more;

    // Check if name is root, if it is, return -1
    if(strcmp(name, "root") == 0)
    {
        printf("check_name: bad file name `%s': there can only be one root\n", name);
        return -1;
    }

    do
    {
        is_more = false;
        for(i = blk_start; i < indexable_count; i += 8)
        {
            //printf("i is %d\n", i);
            if(strcmp((const char *)(filesys + i), name) == 0)
            {
                return i;
            }
        }
        if(filesys[blk_start + 7] != 0xDEADBEEF)
        {
            // There is an extention to this struct dir:
            blk_start = filesys[blk_start + 7] * BLK_SZ_INT;
            filled_count = filesys[blk_start + FILLED_COUNT];
            indexable_count = (filled_count+1)*8 + blk_start;
            is_more = true;
        }
    } while(is_more);

    // Filename was not found
    return 0;
}

// Creates FCB and tells FCB to point at the allocated space
// (the number of 'size' blocks will be pointed to).
// size is given in bytes. Take size/BLK_SZ_BYTE to get the number of blocks
// we need to allocate.
int create_file(const char *file_name, const int size)
{
    // Find out if the filename is too large:
    int namelen = strlen(file_name);
    if(namelen > DIR_NAME_MAX-1)
    {
        fprintf(stderr, "%s: file_name too large (>DIR_NAME_MAX)\n", __func__);
        return -1;
    }

    // Check the size to be allocated. If it's greater than 60 KB, we can't do it.
    if(size > 61440)
    {
        fprintf(stderr, "%s: file size too large (>60 KB)\n", __func__);
        return -1;
    }
    if(size <= 0)
    {
        fprintf(stderr, "%s: file size must be at least 1 byte.\n", __func__);
        return -1;
    }

    // Check to see if name already exists in dir:
    if(check_name(file_name) != 0)
    {
        fprintf(stderr, "%s: %s: file name already exists\n", __func__, file_name);
        return -1;
    }

    // Build the FCB
    // Allow 28 bytes for the file_name, and 4 bytes for holding the size
    free_blks_bounds bnds = find_free_blocks(1);
    int FCB_blk = bnds.start;
    flag_bit(FCB_blk);

    // Figure out the actual number of blocks we will take up with the allocation:
    int block_size = size / BLK_SZ_BYTE;
    if((size % BLK_SZ_BYTE) > 0)
        block_size++;

    // File name is not too large, so put it into the first 28 bytes of the file:
    memmove((char *)(filesys + (FCB_blk * BLK_SZ_INT)), file_name, namelen+1);

    // The 28th-32nd bytes of the block are to hold the size:
    filesys[(FCB_blk * BLK_SZ_INT)+7] = block_size;
    
    // We have 120 ints left to work with. This means we can allocate a max
    // of 120 blocks, or 60 KB.

    bnds = find_free_blocks(block_size);
    if(bnds.start != -1)
    {
        alloc_blocks_FCB(bnds.start, bnds.end, FCB_blk, 0);
    }
    else
    {
        return -1;
    }

    return FCB_blk;
}

void alloc_blocks_FCB(int bnds_start, int bnds_end, int FCB_blk, int start_alloc_at)
{
    int i = (FCB_blk * BLK_SZ_INT) + start_alloc_at + 8;
    int alloc = bnds_start;
    for(;i < (FCB_blk * BLK_SZ_INT)+BLK_SZ_INT && alloc <= bnds_end; i++)
    {
        flag_bit(alloc);
        filesys[i] = alloc;
        alloc++;
    }
}

int rename_f(char *name, char *size, int type)
{
    // Can only remove directories pointed to by the cur_dir
    // name == oldname, size == newname

    int sizelen = strlen(size);

    if(name == NULL || strlen(name) == 0)
    {
        fprintf(stderr, "%s: %s: no such file or directory\n", __func__, name);
        return -1;
    }
    if(size == NULL || sizelen == 0)
    {
        fprintf(stderr, "%s: %s: no new file name given\n", __func__, size);
        return -1;
    }
    if(strcmp(name, ".") == 0 || strcmp(name, "..") == 0 || strcmp(size, ".") == 0
            || strcmp(size, "..") == 0)
    {
        fprintf(stderr, "%s: %s: file name cannot be '.' or '..'\n", __func__, size);
        return -1;
    }

    if(sizelen > DIR_NAME_MAX-1)
    {
        fprintf(stderr, "%s: %s: file name too long (>DIR_NAME_MAX)\n", __func__, size);
        return -1;
    }

    // Check if directory of name 'size' already exists; if so kill the function, throw error.
    if(check_name(size) != 0) // Then the name was found
    {
        fprintf(stderr, "%s: %s: file name already exists in directory\n", __func__, size);
        return -1;
    }
    // name cannot be the directory name, so check for that:
    if(strcmp(name, (char *)(filesys + (cur_dir * BLK_SZ_INT))) == 0)
    {
        fprintf(stderr, "%s: %s: file name already exists in directory\n", __func__, name);
        return -1;
    }

    int i = check_name(name);
    
    if(filesys[i+6] != (unsigned int)type)
    {
	fprintf(stderr, "%s: %s: wrong type\n", __func__, name);
	return -1;
    }

    if(i != 0)
    {
        // name was found in directory.
        memmove((char *)(filesys + i), size, sizelen+1);
        // Change the struct dir's name:
        int dir_addr = filesys[i+7];
        memmove((char *)(filesys + (dir_addr * BLK_SZ_INT)), size, sizelen+1);
        return 0;
    }


    printf("%s: %s: file name not found\n", __func__, name);

    return -1;
}
