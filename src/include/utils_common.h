#ifndef __UTILS_COMMON_H__

#define __UTILS_COMMON_H__

#include <tipos.h>

void *memcpy(void *dest, const void *src, size_t n);

void *memset(void * ptr, int value, size_t num);

int strcmp(char * src, char * dst);
int strncmp(const char *s1, const char *s2, size_t n);
char *strstr(const char *in, const char *str);
int strlen(const char* str);
int strtoi(const char *str);

int power(int x, int y);

#endif
