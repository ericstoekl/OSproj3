/* Erich Stoekl, Penn State University, 2012
 * ems5311@psu.edu
 *
 * stack.h - functions for stack  
 * Used for print function
 *
 * adapted from http://www.cs.utsa.edu/~wagner/CS2213/stack/stack.html
 * with permission
 */
#ifndef _DIR_STACK_H
#define _DIR_STACK_H

void dir_stck_push(int c);
char dir_stck_pop();
int dir_stck_empty();
int dir_stck_full();

#endif
