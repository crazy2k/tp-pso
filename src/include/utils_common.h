#ifndef __UTILS_COMMON_H__

#define __UTILS_COMMON_H__

#include <tipos.h>

void *memcpy(void *dest, const void *src, size_t n);
void *memset(void * ptr, int value, size_t num);

int strcmp(const char * src, const char * dst);
int strncmp(const char *s1, const char *s2, size_t n);
char *strstr(const char *in, const char *str);
int strlen(const char* str);
int strtoi(const char *str);
char *strcpy(char *s1, const char *s2);
char *strcat(char *s1, const char *s2);
char *strncat(char *s1, const char *s2, size_t n);

int power(int x, int y);
long max(long var1, long var2);
long min(long var1, long var2);

int align_to_lower(int number, int mult);
int align_to_next(int number, int mult);

#endif
