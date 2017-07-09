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
#include <serial.h>
#include <fs.h>
#include <hdd.h>
#include <mm.h>

#define PF_ISR_P 0x1
#define PF_ISR_WR 0x2
#define PF_ISR_US 0x4
#define PF_ISR_RSVD 0x8
#define PF_ISR_ID 0xC

typedef void (*isr_t)(uint32_t index, uint32_t error_code, task_state_t *st);

// IDT e idtr
uint64_t idt[IDT_LENGTH] = {0};
idtr_t idtr = { .size = sizeof(idt) - 1, .addr = idt };
isr_t isrs[IDT_LENGTH] = {NULL};

extern void (*idt_stateful_handlers[IDT_LENGTH])();

static int register_isr(uint32_t index, isr_t isr);
static void timer_isr(uint32_t index, uint32_t error_code, task_state_t *st);
static void keyboard_isr(uint32_t index, uint32_t error_code,
    task_state_t *st);
static void serial_isr(uint32_t index, uint32_t error_code, task_state_t *st);
static void syscall_caller(uint32_t index, uint32_t error_code, task_state_t
    *st);
static void primary_hdd_isr(uint32_t index, uint32_t error_code, task_state_t
    *st);
static void page_fault_isr(uint32_t index, uint32_t error_code, 
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

    register_isr(IDT_INDEX_TIMER, timer_isr);
    register_isr(IDT_INDEX_KB, keyboard_isr);
    register_isr(IDT_INDEX_COM13, serial_isr);
    register_isr(IDT_INDEX_SYSC, syscall_caller);
    register_isr(IDT_INDEX_HDD_PRIMARY, primary_hdd_isr);
    register_isr(IDT_INDEX_PF, page_fault_isr);


    // Cargamos la IDT
    lidt(&idtr);

    remap_PIC(PIC1_OFFSET, PIC2_OFFSET);

    // Desenmascaramos interrupciones en el PIC
    pic_clear_mask(PIC_TIMER);
    pic_clear_mask(PIC_KB);
    pic_clear_mask(PIC_SLAVE);
    pic_clear_mask(PIC_PRIMARY_HDD);
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

/* Registra una rutina de atencion ``isr`` para la excepcion/interrupcion cuyo
 * indice en la IDT es ``index``.
 *
 * Si el indice pasado es incorrecto, se devuelve IDT_BAD_INDEX. Si la
 * excepcion/interrupcion ya tiene una rutina de atencion registrada, devuelve
 * IDT_BUSY. Si no hay problemas con el registro, devuelve 0.
 */
static int register_isr(uint32_t index, isr_t isr) {

    // Chequeamos si el numero de irq es valido
    if ((index < 0) || (index > IDT_LAST_INDEX))
        return IDT_BAD_INDEX;

    // Chequeamos si la interrupcion ya esta siendo atendida
    if (isrs[index] != NULL)
        return IDT_BUSY;

    isrs[index] = isr;

    return 0;
}

static void default_isr(uint32_t index, uint32_t error_code, task_state_t *st) {
    debug_kernelpanic(st, index, error_code);
}

void idt_handle(uint32_t index, uint32_t error_code, task_state_t *st) {
//    debug_printf("Code: %d\n", index);
    outb(PIC1_COMMAND, OCW2);
    outb(PIC2_COMMAND, OCW2);

    if (isrs[index] == NULL) {
        default_isr(index, error_code, st);
    }
    else {
        isrs[index](index, error_code, st);
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
            break;
        case SYSCALLS_NUM_SEEK:
            st->eax = sys_seek(st->ebx, st->ecx);
            break;
        case SYSCALLS_NUM_CLOSE:
            st->eax = sys_close(st->ebx);
            break;
        case SYSCALLS_NUM_OPEN:
            st->eax = sys_open((char *)st->ebx, st->ecx);
            break;
        case SYSCALLS_NUM_STAT:
            st->eax = fs_stat((char *)st->ebx, (stat_t *)st->ecx);
            break;
        case SYSCALLS_NUM_CON_CTL:
            con_ctl((con_chardev *) loader_get_file(st->ebx), st->ecx);
            break;
        case SYSCALLS_NUM_RUN:
            st->eax = sys_run((char *)st->ebx);
            break;
        case SYSCALLS_NUM_PIPE:
            st->eax = sys_pipe((int *)st->ebx);
            break;
        case SYSCALLS_NUM_FORK:
            st->eax = sys_fork(st);
            break;
        case SYSCALLS_NUM_SHARE_PAGE:
            st->eax = sys_share_page((void *)st->ebx);
            break;
    }
}

static void timer_isr(uint32_t index, uint32_t error_code, task_state_t *st) {
    pid pid = sched_tick();
    loader_switchto(pid);
}

static void keyboard_isr(uint32_t index, uint32_t error_code,
    task_state_t *st) {
    kb_process_byte(inb(0x60));
}

static void serial_isr(uint32_t index, uint32_t error_code, task_state_t *st) {
    serial_recv();
}

static void primary_hdd_isr(uint32_t index, uint32_t error_code,
    task_state_t *st) {

    hdd_primary_isr();
}

static void page_fault_isr(uint32_t index, uint32_t error_code, task_state_t *st) {
    void* page = (void *) rcr2();

    if (!(error_code & PF_ISR_P) && mm_is_swapped_page(page)) {
        debug_printf("page_fault_isr: swapped page\n");
        mm_swap_page_in(page);
    } else if (!(error_code & PF_ISR_P) && mm_is_requested_page(page)) {
        debug_printf("page_fault_isr: requested page\n");
        if (mm_load_requested_page(page) == NULL)
            loader_exit();
    }
    else if ((error_code & PF_ISR_WR) && mm_is_cow_page(page)) {
        debug_printf("page_fault_isr: COW page\n");
        if (mm_load_cow_page(page) < 0) {
            debug_printf("page_fault_isr: COW page error\n");
            loader_exit();
        }
    }
    else {
        debug_printf("page_fault_isr: process: %d addr: %x errcode: %x\n",
            sched_get_current_pid(), page, error_code);
        debug_kernelpanic(st, index, error_code);
    }
}

