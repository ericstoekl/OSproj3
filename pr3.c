/* CMPSC 473, Project 3, starter kit
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

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

/*    create_struct_dir("root1");
    create_struct_dir("root2");
    create_struct_dir("root3");
    create_struct_dir("root4");
*/
    return 0;
}


int do_print(char *name, char *size)
{
    if (debug) printf("%s\n", __func__);
    return -1;
}


int do_chdir(char *name, char *size)
{
    if (debug) printf("%s\n", __func__);
    return -1;
}

// Create a new struct_directory, as well as a new directory entry in the
// current working directory (cur_dir). 
int do_mkdir(char *name, char *size)
{
    if (debug) printf("%s\n", __func__);

    if(check_name(name) != 0)
        return -1;

    int newdir_addr = create_struct_dir(name);

    if(debug) printf("created new directory at block %d\n", newdir_addr);

    // Add a directory entry:
    if(add_dir_entry(name, newdir_addr) != 0)
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
    return -1;
}


int do_mkfil(char *name, char *size)
{
    if (debug) printf("%s\n", __func__);
    return -1;
}


int do_rmfil(char *name, char *size)
{
    if (debug) printf("%s\n", __func__);
    return -1;
}


int do_mvfil(char *name, char *size)
{
    if (debug) printf("%s\n", __func__);
    return -1;
}


int do_szfil(char *name, char *size)
{
    if (debug) printf("%s\n", __func__);
    return -1;
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
