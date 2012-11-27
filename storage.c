/* Erich Stoekl, Penn State University, 2012
 * ems5311@gmail.com
 *
 * storage.c
 *
 * Implements functions given in storage.h
 */

#include <malloc.h>
#include <math.h>
#include <stdlib.h>

#include "storage.h"

/*
#define BLK_SZ_INT 128
#define BLK_SZ_BYTE 512
#define MAX_BIT_VAL
*/

// Global unsigned int pointer, points to entire filesystem storage arena.
unsigned int *filesys;


int init_filesys()
{

    // First, we allocate the file system, 2 megabytes total.
    // The reason we are going with 2 megabytes is because the first 512 byte block
    // will be a bitmap. 512 bytes is 4096 bits, so one bit can map to each 512 byte
    // block in the entire storage arena (there are 4096 512-byte blocks in 2 megs).
    filesys = (unsigned int *)malloc(2097152);
    if(filesys == NULL)
        return -1;

    // Set up the first block (first 128 ints) as the bitmap block.
    int i;
    for(i = 0; i < BLK_SZ_INT; i++)
        *(filesys+i) = 0;
    
    // The very first (zeroth) block should be flagged in the bitmap,
    // because it's taken by the bitmap itself!
    flag_bit(0);

    return 0;

}

// Flags a single bit in the bitmap vector as "allocated" (1).
// If that bit is already flagged, it does nothing.
void flag_bit(unsigned int blocknum)
{
    if(blocknum >= 4096)
    {
        fprintf(stderr, "ERROR: flag_bit(): blocknum %d\n", blocknum);
        exit(-1);
    }
    // Divide blocknum by 32 to get the bitmap block integer index:
    unsigned int i_index = blocknum / 32;
    unsigned int b_index = blocknum % 32;

    b_index = (unsigned int)pow(2, b_index); // Take 2^b_index to isolate the bit to flag

    filesys[i_index] = filesys[i_index] | b_index;
}

// Finds howmany blocks, returns the start index of the free blocks and end index.
free_blks_bounds find_free_blocks(unsigned int howmany)
{
    if(howmany < 1)
    {
        fprintf(stderr, "find_free_blocks(): block request must be > 0\n");
        return (free_blks_bounds){-1, -1};
    }

    // Search through the bitmap in linear time:
    int i, freecount;
    unsigned int j, ander;

    freecount = 0;
    for(i = 0; i < BLK_SZ_INT; i++)
    {
        for(j = 0; j < 32; j++)
        {
            if(freecount == (int)howmany)
            {
                // We've found howmany contiguous free blocks!
                return (free_blks_bounds){i*32 + j - howmany, i*32 + j};
            }

            ander = (unsigned int)pow(2, j);
            if((filesys[i] & ander) == 0)
                freecount++;
            else
                freecount = 0;
        }
    }
    
    // No free blocks have been found in the system!
    fprintf(stderr, "find_free_blocks(): %d free blocks unavailable\n", howmany);
    return (free_blks_bounds){-1, -1};
}

// We will need a block addressing scheme.
// There are 128 ints per block.
