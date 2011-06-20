#ifndef __USER_IO_H__

#define __USER_IO_H__

void write_hex(uint32_t fd, uint32_t num);
void write_decimal(uint32_t fd, uint32_t num);
void write_str(uint32_t fd, char* src);

void println(uint32_t fd, char* src);

int scanln(uint32_t fd, char* dest, int size);

int scanln_autocomp(uint32_t fd, char* dest, int size, int (*autocompl) (char *read_buf, int pos)) ;

#endif
