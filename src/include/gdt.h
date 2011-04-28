#ifndef __GDT_H__
#define __GDT_H__

#include <tipos.h>

/* GDT
 * ===
 * 
 * Convenciones:
 * - Todas las macros relacionadas con la GDT comienzan con el
 *   prefijo ``GDT_``.
 * - Aquellas relacionadas con descriptores en la GDT, llevan el prefijo
 *   ``GDT_DESC_``. El resto del nombre de la macro lleva el nombre dado en
 *   los manuales de Intel, con la salvedad de que los espacios se reemplazan
 *   por ``_``, el caracter ``/`` se omite y todas las letras van en mayuscula.
 * - Los flags, por comodidad, tienen solo el prefijo ``GDT_F``.
 *   Los flags para tipos de segmentos de aplicaciones siguen con ``_DATA_X``
 *   o ``_CODE_X`` segun sean de datos o de codigo, donde X representa el
 *   nombre que se le da en los manuales a cada bit que compone el tipo.
 *   Los flags para tipos de sistema siguen con las siglas correspondientes al
 *   tipo que representan segun el nombre que se le da en los manuales de
 *   Intel.
 */
#define GDT_DESC_BASE(dir) ( ((uint64_t)(dir) & __LOW16_BITS__) << 16 | \
	((uint64_t)(dir) & __16_23_BITS__) << (32 - 16) | \
	((uint64_t)(dir) & __24_31_BITS__) << 32 )

#define GDT_DESC_LIMIT(dir) ( ((uint64_t)(dir) & __LOW16_BITS__) | \
	((uint64_t)(dir) & __16_19_BITS__) << 32 )

#define GDT_DESC_G (((uint64_t) 1) << (32 + 23))
#define GDT_DESC_DB (((uint64_t) 1) << (32 + 22))
#define GDT_DESC_L (((uint64_t) 1) << (32 + 21))
#define GDT_DESC_P (((uint64_t) 1) << (32 + 15))
#define GDT_DESC_DPL(level) (((uint64_t) level) << (32 + 13))
#define GDT_DESC_S (((uint64_t) 1) << (32 + 12))
#define GDT_DESC_TYPE(type) ((uint64_t)(type) << (32 + 8))

// Flags para tipos de aplicaciones
#define GDT_F_DATA_A    0x1 // accessed
#define GDT_F_DATA_W    0x2 // write-enable
#define GDT_F_DATA_E    0x4 // expansion-direction
#define GDT_F_CODE_A    0x9 // accessed
#define GDT_F_CODE_R    0xA // read enable
#define GDT_F_CODE_C    0xC // conforming

// Flags para tipos de sistema
#define GDT_F_16BTA     0x1 // 16-bit TSS (Available)
#define GDT_F_LDT       0x2 // LDT
#define GDT_F_16BTB     0x3 // 16-bit TSS (Busy)
#define GDT_F_16BCG     0x4 // 16-bite Call Gate
#define GDT_F_TG        0x5 // Task Gate
#define GDT_F_16BIG     0x6 // 16-bit Interrupt Gate
#define GDT_F_16BTG     0x7 // 16-bit Trap Gate
#define GDT_F_32BTA     0x9 // 32-bit TSS (Available)
#define GDT_F_32BTB     0xB // 32-bit TSS (Busy)
#define GDT_F_32BCG     0xC // 32-bit Call Gate
#define GDT_F_32BIG     0xE // 32-bit Interrupt Gate
#define GDT_F_32BTG     0xF // 32-bit Trap Gate

#define GDT_NULL ((uint64_t) 0)

#define GDT_SEGSEL(rpl, index) \
    (((rpl) & __LOW2_BITS__) | ((index) << 3))

typedef struct { 
    uint16_t size __attribute__((packed));
    void *addr __attribute__((packed));
} gdtr_t; 

extern gdtr_t gdtr;

extern uint64_t gdt[];

#define GDT_INDEX_NULL 0x0
#define GDT_INDEX_KERNEL_CS 0x1
#define GDT_INDEX_KERNEL_DS 0x2
#define GDT_INDEX_USER_CS 0x3
#define GDT_INDEX_USER_DS 0x4
#define GDT_INDEX_TSS 0x5

void gdt_init(void);


#endif
