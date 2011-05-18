#ifndef __ISR_H__
#define __ISR_H__


#include <gdt.h>
#include <idt.h>


typedef struct {
    /*
     * Registros a ser cargados usando ``popa``
     */
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;

    // Espaciado: ``popa`` se saltea estos 32 bits del stack, asi que los
    // usamos para ``esp``
    uint32_t esp;

    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;

    /*
     * Registros de segmento
     */
    uint16_t gs;
    uint16_t :16;
    uint16_t fs;
    uint16_t :16;
    uint16_t es;
    uint16_t :16;
    uint16_t ds;
    uint16_t :16;
    //uint16_t ss;
    //uint16_t :16;

    /*
     * Estado a ser cargado por ``iret``
     */
    uint32_t eip;
    uint16_t cs;
    uint16_t :16;
    uint32_t eflags;

    /*
     * Datos del stack que ``iret`` solo carga si hay cambio de nivel
     */
    uint32_t org_esp;
    uint16_t org_ss;
    uint16_t :16;
} task_state_t;


#endif
