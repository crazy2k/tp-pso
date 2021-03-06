#ifndef __UTILS_COMMON_H__

#define __UTILS_COMMON_H__

#include <tipos.h>

void *memcpy(void *dest, const void *src, size_t n);
void *memset(void * ptr, int value, size_t num);

int strcmp(const char * src, const char * dst);
int strncmp(const char *s1, const char *s2, size_t n);
char *strchr(const char *s, int c);
char *strstr(const char *in, const char *str);
int strlen(const char* str);
int strtoi(const char *str);
void itohex(uint32_t n, char *s);
void itostr(int n, char *s);
void reverse(char *s);
char *strcpy(char *s1, const char *s2);
char *strcat(char *s1, const char *s2);
char *strncat(char *s1, const char *s2, size_t n);

int isdigit(int var);
int isnumeric(const char *str);

int power(int x, int y);
long max(long var1, long var2);
long min(long var1, long var2);

int align_to_lower(int number, int mult);
int align_to_next(int number, int mult);

#endif
