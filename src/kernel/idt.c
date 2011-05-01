#include <tipos.h>
#include <i386.h>
#include <gdt.h>
#include <isr.h>
#include <idt.h>
#include <pic.h>

#define IDT_INT IDT_ATTR_P | IDT_ATTR_S_ON | IDT_ATTR_D_32 | IDT_ATTR_TYPE_INT
#define IDT_EXP IDT_ATTR_P | IDT_ATTR_S_ON | IDT_ATTR_D_32 | IDT_ATTR_TYPE_EXP


int idt_set_isr(uint32_t index, void (*handler)(), uint64_t attr);

uint64_t idt[IDT_LENGTH] = {0};
idtr_t idtr = { .size = sizeof(idt) - 1, .addr = idt };

void idt_init(void) {
    // Cargamos la IDT
    lidt(&idtr);

    remap_PIC(PIC1_OFFSET, PIC2_OFFSET);
    
	return;
}

/* ``idt_register()`` existe por compatibilidad con el codigo original, pero
 * en nuestro codigo utilizamos ``idt_set_isr()``. La diferencia entre ambas
 * funciones es que 1) la segunda devuelve codigos de error y 2) la segunda
 * recibe atributos de 64 bits en lugar de 32. Esto ultimo nos permite
 * reutilizar macros que teniamos escritas en Zafio.
 */
void idt_register(int intr, void (*isr)(void), int attr) {
    idt_set_isr(intr, isr, (((uint64_t)attr) << 32));
}

/* Registra un handler (``handler``)  para la excepcion/interrupcion cuyo
 * indice en la IDT es ``index``.
 *
 * Si el indice pasado es incorrecto, se devuelve IDT_BAD_INDEX. Si la
 * excepcion/interrupcion ya tiene un handler registrado, devuelve IDT_BUSY. Si
 * no hay problemas con el registro, se devuelve 0.
 *
 * El handler se registra con los atributos pasados en ``attr``.
 */
int idt_set_isr(uint32_t index, void (*handler)(), uint64_t attr) {
    // Chequeamos si el numero de irq es valido
    if ((index < 0) || (index > IDT_LAST_INDEX))
        return IDT_BAD_INDEX;

    // Chequeamos si la interrupcion ya esta siendo atendida
    if (idt[index] & IDT_DESC_P)
        return IDT_BUSY;

//    uint64_t dpl = (index == IDT_INDEX_SYSC) ? IDT_DESC_DPL(3) :
//        IDT_DESC_DPL(0);

    // Escribimos el descriptor en la IDT
    idt[index] = IDT_DESC_SEL(GDT_SEGSEL(0x0, GDT_INDEX_KERNEL_CS)) |
        IDT_DESC_OFFSET(handler) | attr;

// | IDT_DESC_P | IDT_DESC_D | IDT_DESC_INT |
//        dpl;

    return 0;
}

