/* Project 3, CMPSC 473
 * Erich Stoekl, Joong Hun Kwak
 * ems5311@psu.edu, jwk5221@psu.edu
 *
 * pr3.c
 *
 * Main entry point for filesystem project, Project 3
 * Parses input from stdin, then runs function based on that
 * input to perform specified task.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>

#include "f_system.h"
#include "storage.h"
#include "dir_stack.h"

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

// Stack values:
extern int *dir_stck_items;
extern int dir_stck_top;

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
        if (debug) printf("\n:%s:%s:%s:\n", cmd, fnm, fsz);

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
    
    int print_dir; // The directory that we will print is stored in this var.
                   // At first, it will be cur_dir.
                   // Then it will take on whatever directory it gets from the dir_stack.

    // Do while the stack is not empty. Uses a stack to avoid having to do recursion.
    // This is to facilitate 'ls -lR' style directory printing.
    do
    {
        printf("\n");
        if(dir_stck_empty() != 0) // The stack is empty, set print_dir to cur_dir:
        {
            print_dir = cur_dir;
        }
        else
        {
            print_dir = dir_stck_pop();
        }

        unsigned int i = (print_dir * BLK_SZ_INT);

        printf("Currently looking at directory `%s'\n", (char *)(filesys + i));

        int counter = 0;
        int filecount = filesys[i + FILLED_COUNT];
        bool is_more;

        // print contents of current directory:
        do
        {
            is_more = false;
            for(i += 8; counter < filecount; i += 8)
            {
                if(filesys[i] == 0xDEADBEEF) continue;
                if(filesys[i+6] == IS_DIR)
                {
                    printf("Dir %s, address: %d\n", (char *)(filesys + i), filesys[i+7]);
                    if(strcmp((char *)(filesys + i), ".") != 0 && strcmp((char *)(filesys + i), "..") != 0)
                        dir_stck_push(filesys[i+7]); // Push directory onto stack so we can explore next
                                                     // do while cycle.
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
                counter++;
            }
            
            if(filesys[print_dir * BLK_SZ_INT + 7] != 0xDEADBEEF)
            {
                // Then there is an extention struct dir:
                print_dir = filesys[print_dir * BLK_SZ_INT + 7];
                i = print_dir * BLK_SZ_INT;
                filecount = filesys[i + FILLED_COUNT];
                counter = 0;
                is_more = true;
            }
        } while(is_more);
    } while(dir_stck_empty() == 0);

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
    (void)size;

    if (debug) printf("%s\n", __func__);

    if(strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
    {
        fprintf(stderr, "%s: %s: cannot remove directory\n", __func__, name);
        return -1;
    }

    // If the filled count for the given directory is 2 or less, than delete it.
    int i = check_name(name);
    if(i != 0 && filesys[i + 6] == IS_DIR) // Then the given name is a directory.
    {
        int dir_index = filesys[i + 7];
        if(filesys[dir_index * BLK_SZ_INT + FILLED_COUNT] <= 2)
        {
            // Then we can remove this dir.
            // Turn the directory entry in cur_dir into deadbeef:
            int j;
            for(j = 0; j < 8; j++)
                filesys[i + j] = 0xDEADBEEF;
            // Unallocate in bitmap
            flag_bit(dir_index);

            // Decrement the filled count in the cur_dir:
            filesys[cur_dir * BLK_SZ_INT + FILLED_COUNT]--;
            return 0;
        }

    }
    
    fprintf(stderr, "%s: %s: cannot remove directory\n", __func__, name);
    return -1;
}


int do_mvdir(char *name, char *size)
{
    if (debug) printf("%s\n", __func__);
    
    if(rename_f(name, size, IS_DIR) != 0)
        return -1;

    return 0;
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
    (void)size;
    if (debug) printf("%s\n", __func__);

    // Find out if the file actually exists
    int i = check_name(name);
    if(i != 0 && filesys[i + 6] == IS_FILE) // Then the given name is a file.
    {
        int FCB_index = filesys[i + 7];
        int file_size = filesys[FCB_index * BLK_SZ_INT + 7];
        // We now have the block index of the FCB.
        // We should de-allocate all the data that the FCB points to.
        int j;
        for(j = 0; j < file_size; j++)
        {
            flag_bit(filesys[FCB_index * BLK_SZ_INT + 8 + j]);
        }
        // Deallocate the FCB itself:
        flag_bit(FCB_index);

        // Remove the entry pointing to the now defunct FCB in the cur_dir:
        // Turn the directory entry in cur_dir into deadbeef:
        for(j = 0; j < 8; j++)
            filesys[i + j] = 0xDEADBEEF;

        // Decrement the file count in cur_dir:
        filesys[cur_dir * BLK_SZ_INT + FILLED_COUNT] --;

        return 0;
    }
    
    fprintf(stderr, "%s: %s: cannot remove file\n", __func__, name);
    return -1;
}


int do_mvfil(char *name, char *size)
{
    if (debug) printf("%s\n", __func__);

    if(rename_f(name, size, IS_FILE) != 0)
        return -1;

    return 0;
}


int do_szfil(char *name, char *size)
{
    if (debug) printf("%s\n", __func__);

    if(size == NULL || strlen(size) == 0) return -1;

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
        // Unallocalte all entries beyond blk_sz point in the FCB:
        int i = (FCB_blk * BLK_SZ_INT) + 8 + blk_sz;
        for(; i < ((FCB_blk * BLK_SZ_INT) + 8 + cur_size); i++)
            flag_bit(filesys[i]);

        // Rewrite FCB size:
        filesys[(FCB_blk * BLK_SZ_INT) + 7] = blk_sz;
    }
    
    /* else The inputted size is equal to the current size, do nothing. */

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
