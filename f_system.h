/* Erich Stoekl, Penn State University, 2012
 * ems5311@psu.edu
 *
 * Functions for dealing with:
 * - struct directory creation and management
 * - FCB creation and management
 */

#ifndef _F_SYSTEM_H
#define _F_SYSTEM_H

// Create struct directory:
// A directory entry in the filesystem, contains pointers to all
// things in the directory.
// Ex: root directory contains file a, file b, directory etc
// so the struct directory object (called 'root') points to FCB 'a', FCB 'b',
// and struct directory 'etc'
int create_struct_dir(const char* dir_name);




#endif
