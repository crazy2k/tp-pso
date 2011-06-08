#include <tipos.h>
#include <i386.h>
#include <gdt.h>
#include <isr.h>
#include <idt.h>
#include <pic.h>
#include <debug.h>
#include <loader.h>
#include <sched.h>
#include <syscalls.h>
#include <con.h>
#include <utils.h>
#include <kb.h>


// IDT e idtr
uint64_t idt[IDT_LENGTH] = {0};
idtr_t idtr = { .size = sizeof(idt) - 1, .addr = idt };

extern void (*idt_stateful_handlers[IDT_LENGTH])();

static void syscall_caller(uint32_t index, uint32_t error_code, task_state_t
    *st);
static void keyboard_isr(uint32_t index, uint32_t error_code,
    task_state_t *st);

void idt_init(void) {
    // Configuramos los handlers en la IDT
    int i;
    for (i = 0; i <= IDT_LAST_INDEX; i++) {
        // En principio, a todos los indices les registramos stateful handlers
        if (i == IDT_INDEX_SYSC)
            idt_set_handler(IDT_INDEX_SYSC, idt_stateful_handlers[IDT_INDEX_SYSC],
                IDT_DESC_P | IDT_DESC_D | IDT_DESC_INT | IDT_DESC_DPL(0x3));
        else
            idt_set_handler(i, idt_stateful_handlers[i], IDT_DESC_P | IDT_DESC_D |
                IDT_DESC_INT | IDT_DESC_DPL(0));
    }

    // Cargamos la IDT
    lidt(&idtr);

    remap_PIC(PIC1_OFFSET, PIC2_OFFSET);

    // Desenmascaramos interrupciones en el PIC
    outb(PIC1_DATA, PIC_ALL_ENABLED);
}

/* ``idt_register()`` existe por compatibilidad con el codigo original, pero
 * en nuestro codigo utilizamos ``idt_set_handler()``. La diferencia entre
 * ambas funciones es que 1) la segunda devuelve codigos de error y 2) la
 * segunda recibe atributos de 64 bits en lugar de 32. Esto ultimo nos permite
 * reutilizar macros que teniamos escritas en Zafio.
 */
void idt_register(int intr, void (*isr)(void), int attr) {
    idt_set_handler(intr, isr, (((uint64_t)attr) << 32));
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
int idt_set_handler(uint32_t index, void (*handler)(), uint64_t attr) {
    // Chequeamos si el numero de irq es valido
    if ((index < 0) || (index > IDT_LAST_INDEX))
        return IDT_BAD_INDEX;

    // Chequeamos si la interrupcion ya esta siendo atendida
    if (idt[index] & IDT_DESC_P)
        return IDT_BUSY;

    // Escribimos el descriptor en la IDT
    idt[index] = IDT_DESC_SEL(GDT_SEGSEL(0x0, GDT_INDEX_KERNEL_CS)) |
        IDT_DESC_OFFSET(handler) | attr;

    return 0;
}

void idt_handle(uint32_t index, uint32_t error_code, task_state_t *st) {
    outb(PIC1_COMMAND, OCW2);

    if (index == IDT_INDEX_TIMER) {
        pid pid = sched_tick();
        loader_switchto(pid);
    }
    else if (index == IDT_INDEX_KB)
        keyboard_isr(index, error_code, st);

    else if (index == IDT_INDEX_SYSC) {
        syscall_caller(index, error_code, st);
    }
    else {
        breakpoint();
        debug_kernelpanic(st, index, error_code);
    }
}

static void syscall_caller(uint32_t index, uint32_t error_code, task_state_t
    *st) {
    // eax indica el numero de la llamada al sistema
    switch (st->eax) {
        case SYSCALLS_NUM_EXIT:
            sys_exit();
            break;
        case SYSCALLS_NUM_GETPID:
            st->eax = sys_getpid();
            break;
        case SYSCALLS_NUM_PALLOC:
            st->eax = (uint32_t)sys_palloc();
            break;
        case SYSCALLS_NUM_READ:
            st->eax = sys_read(st->ebx, (void *)st->ecx, st->edx);
            break;
        case SYSCALLS_NUM_WRITE:
            st->eax = sys_write(st->ebx, (void *)st->ecx, st->edx);
        case SYSCALLS_NUM_SEEK:
            st->eax = sys_seek(st->ebx, st->ecx);
        case SYSCALLS_NUM_CLOSE:
            st->eax = sys_close(st->ebx);
            break;
        case SYSCALLS_NUM_OPENCON:
            st->eax = con_open();
            break;
        case SYSCALLS_NUM_NEXTCON:
            con_focus(con_get_current_console()->next);
            break;
    }
}

static void keyboard_isr(uint32_t index, uint32_t error_code,
    task_state_t *st) {

    kb_process_byte(inb(0x60));
    breakpoint();
}
