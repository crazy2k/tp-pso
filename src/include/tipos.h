#ifndef __LIBPSO_TIPOS_H__
#define __LIBPSO_TIPOS_H__

typedef unsigned char      uint_8;
typedef unsigned short     uint_16;
typedef unsigned int       uint_32;
typedef unsigned long long uint_64;

typedef signed char      sint_8;
typedef signed short     sint_16;
typedef signed int       sint_32;
typedef signed long long sint_64;

typedef int bool;

typedef sint_32 pid;

#define FALSE 0
#define TRUE 1
#define NULL 0

/*
 * Mascaras
 */

#define __LOW2_BITS__ 0x00000003
#define __LOW16_BITS__ 0x0000FFFF
#define __LOW12_BITS__ 0x00000FFF
#define __LOW28_BITS__ 0x0FFFFFFF
#define __HIGH16_BITS__ 0xFFFF0000
#define __16_23_BITS__ 0x00FF0000
#define __24_31_BITS__ 0xFF000000
#define __16_19_BITS__ 0x000F0000
#define __12_31_BITS__ 0xFFFFF000

#define __22_31_BITS__ 0xFFC00000
#define __12_21_BITS__ 0x003FF000


/* 
 * Aliases para compatibilidad con codigo de Zafio
 */

typedef uint_8      uint8_t;
typedef uint_16     uint16_t;
typedef uint_32     uint32_t;
typedef uint_64     uint64_t;

typedef sint_8      int8_t; 
typedef sint_16     int16_t; 
typedef sint_32     int32_t; 
typedef sint_64     int64_t; 

typedef uint_32     size_t;
#endif
