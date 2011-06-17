#ifndef __USER_UTILS_H__

#define __USER_UTILS_H__

#include <user/tipos.h>

int strcmp(char* left, char* right);
int strlen(char* str);
void *memcpy(void *dest, const void *src, int n);
int power(int x, int y);
int strtoi(char *str);

#endif
