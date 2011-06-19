#ifndef __USER_IO_H__

#define __USER_IO_H__


void println(uint32_t fd, char* src, int size);

int scanln(uint32_t fd, char* dest, int size);

int scanln_autocomp(uint32_t fd, char* dest, int size, int (*autocompl) (char *read_buf, int pos)) ;

#endif
