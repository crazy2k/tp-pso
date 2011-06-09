#ifndef __IDT_H__
#define __IDT_H__

#include <gdt.h>

/* Para inicializar la IDT del sistema. */
void idt_init(void);

/* Para registrar una ISR */
void idt_register(int intr, void (*isr)(void), int pl);

int idt_set_handler(uint32_t index, void (*handler)(), uint64_t attr);

extern uint64_t idt[];
typedef gdtr_t idtr_t;
extern idtr_t idtr;

// Indices de interrupciones y excepciones

#define IDT_INDEX_GP    13ul
#define IDT_INDEX_PF    14ul
#define IDT_INDEX_TIMER 32ul
#define IDT_INDEX_KB    33ul
#define IDT_INDEX_COM24 35ul
#define IDT_INDEX_COM13 36ul
#define IDT_INDEX_SYSC  0x30ul

// Campos de descriptores de la IDT

#define IDT_DESC_P ((uint64_t)0x1 << (32 + 15))
#define IDT_DESC_DPL(dpl) ((uint64_t)(__LOW2_BITS__ & (dpl)) << (32 + 13))
#define IDT_DESC_D ((uint64_t)0x1 << (32 + 11))
#define IDT_DESC_INT ((uint64_t)0x6 << (32 + 8))
#define IDT_DESC_TRAP ((uint64_t)0x7 << (32 + 8))
#define IDT_DESC_SEL(segsel) ((__LOW16_BITS__ & (uint64_t)(segsel)) << 16)
#define IDT_DESC_OFFSET(offset) ((uint64_t)(__LOW16_BITS__ & (uint32_t)(offset)) | \
    ((uint64_t)(__HIGH16_BITS__ & (uint32_t)(offset)) << 32))

// Codigos de error y datos varios

#define IDT_BAD_INDEX   0x1
#define IDT_BUSY        0x2

// Recordar que puede hacer falta actualizar estos valores en idt_handlers.S si
// son alterados aqui
#define IDT_LENGTH      256
#define IDT_LAST_INDEX  (IDT_LENGTH - 1)


/*
 * Constantes de los atributos (dejadas por compatibilidad)
 */

/* Masks */
#define IDT_ATTR_P    0x8000
#define IDT_ATTR_DPL  0x6000
#define IDT_ATTR_DPL0 0x0000
#define IDT_ATTR_DPL1 0x2000
#define IDT_ATTR_DPL2 0x4000
#define IDT_ATTR_DPL3 0x6000
#define IDT_ATTR_S    0x1000

#define IDT_ATTR_D    0x0800
#define IDT_ATTR_TYPE 0x0700


/* S Field */
#define IDT_ATTR_S_ON  0x0000
#define IDT_ATTR_S_OFF 0x1000

/* D Field */
#define IDT_ATTR_D_16  0x0000
#define IDT_ATTR_D_32  0x0800

/* TYPE Field */
#define IDT_ATTR_TYPE_INT 0x0600
#define IDT_ATTR_TYPE_EXP 0x0700



#endif
