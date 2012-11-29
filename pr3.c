/* CMPSC 473, Project 3, starter kit
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>

#include "f_system.h"
#include "storage.h"

/*--------------------------------------------------------------------------------*/

extern int debug;                                    // extra output; 1 = on, 0 = off

/*--------------------------------------------------------------------------------*/

/* The input file (stdin) represents a sequence of file-system commands,
 * which all look like     cmd filename filesize
 *
 * command	action
 * -------	------
 *  root	initialize root directory
 *  print	print current working directory and all descendants
 *  chdir	change current working directory
 *                (.. refers to parent directory, as in Unix)
 *  mkdir	sub-directory create (mk = make)
 *  rmdir	              delete (rm = delete)
 *  mvdir	              rename (mv = move)
 *  mkfil	file create
 *  rmfil	     delete
 *  mvfil	     rename
 *  szfil	     resize (sz = size)
 *  #		comment
 */

// The actual file system, as a global unsigned int pointer:
extern unsigned int *filesys;

extern unsigned int cur_dir;

// Keeps track of whether or not fsystem exists:
bool root_called = false;

/* The size argument is usually ignored.
 * The return value is 0 (success) or -1 (failure).
 */
int do_root (char *name, char *size);
int do_print(char *name, char *size);
int do_chdir(char *name, char *size);
int do_mkdir(char *name, char *size);
int do_rmdir(char *name, char *size);
int do_mvdir(char *name, char *size);
int do_mkfil(char *name, char *size);
int do_rmfil(char *name, char *size);
int do_mvfil(char *name, char *size);
int do_szfil(char *name, char *size);

struct action
{
    char *cmd;                                    // pointer to string
    int (*action)(char *name, char *size);        // pointer to function
}


table[] =
{
    { "root",  do_root  },
    { "print", do_print },
    { "chdir", do_chdir },
    { "mkdir", do_mkdir },
    { "rmdir", do_rmdir },
    { "mvdir", do_mvdir },
    { "mkfil", do_mkfil },
    { "rmfil", do_rmfil },
    { "mvfil", do_mvfil },
    { "szfil", do_szfil },
    { NULL, NULL }
};

/*--------------------------------------------------------------------------------*/

void parse(char *buf, int *argc, char *argv[]);

#define LINESIZE 128

/*--------------------------------------------------------------------------------*/

int main(int argc, char *argv[])
{
    // Get rid of compiler warning
    (void)argc;
    (void)argv;

    debug = 1;

    char in[LINESIZE];
    char *cmd, *fnm, *fsz;
    char dummy[] = "";

    int n;
    char *a[LINESIZE];

    while (fgets(in, LINESIZE, stdin) != NULL)
    {
        // commands are all like "cmd filename filesize\n" with whitespace between

        // parse in
        parse(in, &n, a);

        cmd = (n > 0) ? a[0] : dummy;
        fnm = (n > 1) ? a[1] : dummy;
        fsz = (n > 2) ? a[2] : dummy;
        if (debug) printf(":%s:%s:%s:\n", cmd, fnm, fsz);

        if (n == 0) continue;                     // blank line

        int found = 0;
        for (struct action *ptr = table; ptr->cmd != NULL; ptr++)
        {
            if (strcmp(ptr->cmd, cmd) == 0)
            {
                found = 1;
                int ret = (ptr->action)(fnm, fsz);
                if (ret == -1)
                    { printf("  %s %s %s: failed\n", cmd, fnm, fsz); }
                break;
            }
        }
        if (!found)
            { printf("command not found: %s\n", cmd); }
    }

    return 0;
}


/*--------------------------------------------------------------------------------*/

int do_root(char *name, char *size)
{
    // Get rid of compiler warning
    (void)name;
    (void)size;

    if(root_called)
    {
        fprintf(stderr, "%s: Root dir already exists!\n", __func__);
        return -1;
    }

    if (debug) printf("%s\n", __func__);

    // We now call a function that allocates 40 megabytes for the filesys pointer:
    if(init_filesys() == -1)
        return -1;
/*
    free_blks_bounds bnds;
    bnds = find_free_blocks(3);
    printf("free blocks found: %d to %d inclusive\n", bnds.start, bnds.end);
*/
    int root_addr = create_struct_dir("root");
    if(root_addr == -1)
        return -1;

    // Set the current working dir as the root dir:
    cur_dir = root_addr;
    if(debug) printf("root directory is block %d\n", cur_dir);
    
    root_called = true;

    return 0;
}


int do_print(char *name, char *size)
{
    // Get rid of compiler warning
    (void)name;
    (void)size;

    if(!root_called)
    {
        fprintf(stderr, "%s: Root directory must be called first\n", __func__);
        return -1;
    }

    if (debug) printf("%s\n", __func__);

    // 1: Print out contents of cur_dir sequentially
    // If directory, print the directory name and what block it points to
    // If file, print FCB block, and explore FCB block to print file size.
    unsigned int i = (cur_dir * BLK_SZ_INT);

    printf("Current directory is `%s'\n", (char *)(filesys + i));

    for(i += 8; filesys[i] != 0xDEADBEEF; i += 8)
    {
        if(filesys[i+6] == IS_DIR)
        {
            printf("Dir %s, address: %d\n", (char *)(filesys + i), filesys[i+7]);
        }
        else if(filesys[i+6] == IS_FILE)
        {
            // Find out size of the file by looking at FCB:
            int FCB, file_size;
            FCB = filesys[i + 7];
            file_size = filesys[(FCB * BLK_SZ_INT) + 7];
            printf("File %s, address: %d, block size: %d, file size: %d\n", (char *)(filesys + i),
                FCB, file_size, file_size * BLK_SZ_BYTE);
        }
    }

    // 2: Explore sub-directories, and repeat.

    return 0;
}

// Find the directory we want to change to, then set cur_dir as that directory's block addr.
int do_chdir(char *name, char *size)
{
    // Get rid of compiler warning.
    (void)size;

    if(!root_called)
    {
        fprintf(stderr, "%s: Root directory must be called first\n", __func__);
        return -1;
    }

    if (debug) printf("%s\n", __func__);

    // Find the directory, if it exists:
    int i = check_name(name);
    if(i == 0)
    {
        printf("chdir: %s: No such file or directory\n", name);
        return -1;
    }

    if(filesys[i + 6] != IS_DIR)
    {
        printf("chdir: %s: Not a directory\n", name);
        return -1;
    }

    cur_dir = filesys[i + 7];
    if(debug) printf("chdir: cur_dir is now %d\n", cur_dir);
    if(debug) printf("chdir: name of current dir is now %s\n", 
        (char *)(filesys + (cur_dir * BLK_SZ_INT)));

    return 0;
}

// Create a new struct_directory, as well as a new directory entry in the
// current working directory (cur_dir). 
int do_mkdir(char *name, char *size)
{
    (void)size;

    if(!root_called)
    {
        fprintf(stderr, "%s: Root directory must be called first\n", __func__);
        return -1;
    }

    if (debug) printf("%s\n", __func__);

    if(check_name(name) != 0)
    {
        printf("mkdir: cannot create directory `%s': File exists\n", name);
        return -1;
    }

    int newdir_addr = create_struct_dir(name);

    if(debug) printf("created new directory at block %d\n", newdir_addr);

    // Add a directory entry:
    if(add_entry(name, newdir_addr, IS_DIR) != 0)
        return -1;

    return 0;
}


int do_rmdir(char *name, char *size)
{
    if (debug) printf("%s\n", __func__);
    return -1;
}


int do_mvdir(char *name, char *size)
{
    if (debug) printf("%s\n", __func__);
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


int do_mkfil(char *name, char *size)
{
    if (debug) printf("%s\n", __func__);

    if(!root_called)
    {
        fprintf(stderr, "%s: Root directory must be called first\n", __func__);
        return -1;
    }

    int real_size = atoi((const char *)size);

    // 1: create FCB and have it point to allocated space
    int FCB = create_file(name, real_size);
    if(FCB == -1)
        return -1;

    // 2: create entry in pwd pointing to FCB
    if(add_entry(name, FCB, IS_FILE) != 0)
        return -1;

    return 0;
}


int do_rmfil(char *name, char *size)
{
    if (debug) printf("%s\n", __func__);
    return -1;
}


int do_mvfil(char *name, char *size)
{
    if (debug) printf("%s\n", __func__);

    if(do_mvdir(name, size) != 0)
        return -1;

    return 0;
}


int do_szfil(char *name, char *size)
{
    if (debug) printf("%s\n", __func__);

    if(size == NULL) return -1;

    // 1: See if file exists
    // 2: See if size is greater or less than current file size

    int i = check_name(name);
    if(i == 0)
    {
        fprintf(stderr, "%s: %s: File name not found\n", __func__, name);
        return -1;
    }

    int FCB_blk = filesys[i+7];
    int cur_size = filesys[(FCB_blk * BLK_SZ_INT) + 7];

    // input_sz will be size inputted, cur_size is current size of file, both in blocks.
    int input_sz = atoi((const char *)size);
    int blk_sz = input_sz / BLK_SZ_BYTE;
    if(input_sz % BLK_SZ_BYTE != 0)
        blk_sz++;

    // Check the size to be allocated. If it's greater than 60 KB, we can't do it.
    if(input_sz > 61440)
    {
        fprintf(stderr, "%s: file size too large (>60 KB)\n", __func__);
        return -1;
    }

    // if inputted size is bigger than current size, we need to allocate more space.
    if(blk_sz > cur_size)
    {
        // Find out how many more blocks we need:
        int howmany = blk_sz - cur_size;
        free_blks_bounds bnds = find_free_blocks(howmany);

        if(bnds.start == -1)
        {
            fprintf(stderr, "%s: find_free_blocks error\n", __func__);
            return -1;
        }

        alloc_blocks_FCB(bnds.start, bnds.end, FCB_blk, cur_size);

        // Rewrite FCB size:
        filesys[(FCB_blk * BLK_SZ_INT) + 7] = blk_sz;
    }
    else if(blk_sz < cur_size) // We need to unallocate space.
    {
    }
    else // The inputted size is equal to the current size, do nothing.
    {
    }


    return 0;
}


/*--------------------------------------------------------------------------------*/

// parse a command line, where buf came from fgets()

// Note - the trailing '\n' in buf is whitespace, and we need it as a delimiter.

void parse(char *buf, int *argc, char *argv[])
{
    char *delim;                                  // points to first space delimiter
    int count = 0;                                // number of args

    char whsp[] = " \t\n\v\f\r";                  // whitespace characters

    while (1)                                     // build the argv list
    {
        buf += strspn(buf, whsp);                 // skip leading whitespace
        delim = strpbrk(buf, whsp);               // next whitespace char or NULL
        if (delim == NULL)                        // end of line
            { break; }
        argv[count++] = buf;                      // start argv[i]
        *delim = '\0';                            // terminate argv[i]
        buf = delim + 1;                          // start argv[i+1]?
    }
    argv[count] = NULL;

    *argc = count;

    return;
}


/*--------------------------------------------------------------------------------*/
