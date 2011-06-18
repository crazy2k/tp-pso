#ifndef __USER_IO_H__

#define __USER_IO_H__

#include  <user/syscalls.h>


void println(uint32_t fd, char* src, int size);

int scanln(uint32_t fd, char* dest, int size);


#endif
