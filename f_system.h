/* Erich Stoekl, Penn State University, 2012
 * ems5311@psu.edu
 *
 * Functions for dealing with:
 * - struct directory creation and management
 * - FCB creation and management
 */

#ifndef _F_SYSTEM_H
#define _F_SYSTEM_H

#define DIR_NEXT_DIR 7 // The integer index of the field that contains the block
                       // address of the continuation of this directory entry.
#define DIR_NAME_MAX 28
#define DIR_CONTENTS_MAX 120

typedef struct str_dir
{
    char dir_name[DIR_NAME_MAX];              // 28 bytes to hold the name of this directory
    int more_data;                  // block address of block that holds more data (if necessary)
    unsigned int dir_contents[DIR_CONTENTS_MAX]; // 120 ints (15 rows) to account for the rest of the data
                                    // in this directory block.
} struct_dir;

// Create struct directory:
// A directory entry in the filesystem, contains pointers to all
// things in the directory.
// Ex: root directory contains file a, file b, directory etc
// so the struct directory object (called 'root') points to FCB 'a', FCB 'b',
// and struct directory 'etc'
int create_struct_dir(const char* dir_name);

// Adds a directory entry in the current dir (pointed to by cur_dir) 
// for the newly create directory of name 'name' and address 'dir_addr'
int add_dir_entry(const char* name, const int dir_addr);


#endif
