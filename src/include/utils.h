#ifndef __UTILS_H__

#define __UTILS_H__

#include "tipos.h"

void *memcpy(void *dest, const void *src, size_t n);

void *memset(void * ptr, int value, size_t num);

void custom_kpanic_msg(char* custom_msg);
    
#endif
