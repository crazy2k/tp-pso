#ifndef __PIPE_H__
#define __PIPE_H__

#include <tipos.h>
#include <device.h>


void pipe_init(void);

sint_32 pipe_open(chardev* pipes[2]);

// Syscalls
/*
int pipe(int pipes[2]);
*/

#endif
