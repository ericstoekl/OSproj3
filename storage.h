/* Erich Stoekl, Penn State University, 2012
 * ems5311@psu.edu
 *
 * storage.h
 * - Allocates the fourty megabytes the file system requires
 *
 */

#ifndef _STORAGE_H
#define _STORAGE_H

#define BLK_SZ_INT 128
#define BLK_SZ_BYTE 512
#define MAX_BIT_VAL 2147483648

typedef struct strt_end_blck
{
    int start;
    int end;
} free_blks_bounds;

int init_filesys();

void flag_bit(unsigned int blocknum);

// Finds howmany free blocks, returns the start and end addresses.
free_blks_bounds find_free_blocks(unsigned int howmany);

#endif

