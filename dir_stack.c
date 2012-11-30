/* Erich Stoekl, Penn State University, 2012
 * ems5311@psu.edu
 *
 * stack.c - implementation of functions in stack.h
 * Used for print function
 *
 * taken from http://www.cs.utsa.edu/~wagner/CS2213/stack/stack.html
 * with permission
 */

#include "dir_stack.h"

#define MAXSTACK 512
#define EMPTYSTACK -1
int dir_stck_top = EMPTYSTACK;
int dir_stck_items[MAXSTACK];

void dir_stck_push(int c)
{
    dir_stck_items[++dir_stck_top] = c;
}

char dir_stck_pop()
{
    return dir_stck_items[dir_stck_top--];
}

int dir_stck_full()
{
    return dir_stck_top+1 == MAXSTACK;
}

int dir_stck_empty()
{
    return dir_stck_top == EMPTYSTACK;
}

