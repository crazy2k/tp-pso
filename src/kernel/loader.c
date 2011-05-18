#include <loader.h>
#include <isr.h>
#include <mm.h>
#include <utils.h>

#define COMMON_EFLAGS 0x3202
#define USER_STACK 0xC0000000

/*
 * TSS
 */
typedef struct {
    uint16_t prev;  // Previous Task Link
    uint16_t :16;
    uint32_t esp0;  // Stack pointer (nivel 0)
    uint16_t ss0;   // Stack segment (nivel 0)
    uint16_t :16;
    uint32_t esp1;  // Stack pointer (nivel 1)
    uint16_t ss1;   // Stack segment (nivel 1)
    uint16_t :16;
    uint32_t esp2;  // Stack pointer (nivel 2)
    uint16_t ss2;   // Stack segment (nivel 2)
    uint16_t :16;
    uint32_t cr3;   // Page directory base register
    uint32_t eip;
    uint32_t eflags;

    // Registros de proposito general
    uint32_t eax;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t esp;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;

    // Registros de segmento
    uint16_t es;
    uint16_t :16;
    uint16_t cs;
    uint16_t :16;
    uint16_t ss;
    uint16_t :16;
    uint16_t ds;
    uint16_t :16;
    uint16_t fs;
    uint16_t :16;
    uint16_t gs;
    uint16_t :16;
    uint16_t ldt;
    uint16_t :16;

    uint16_t t:1;   // Debug trap flag
    uint16_t :15;
    uint16_t io;    // I/O map base address
}__attribute__((__packed__, aligned (8))) tss_t;

typedef struct pcb pcb;
struct pcb {
    // Direccion virtual y fisica del directorio de paginas en cualquier
    // espacio de direcciones
    void *pd;
    // Datos sobre el stack de kernel de la tarea
    void *kernel_stack;
    void *kernel_stack_limit;
    void *kernel_stack_pointer;

    pcb *next;
    pcb *prev;
};


static void initialize_task_state(task_state_t *st, void *entry_point,
    void *stack_pointer, int pl);



static pcb pcbs[MAX_PID];
static pcb *free_pcbs;
static pcb *pcbs_list;

void loader_init(void) {
    memset(pcbs, 0, sizeof(pcbs));

    free_pcbs = NULL;
    int i;
    for (i = 0; i < MAX_PID; i++) {
        APPEND(&free_pcbs, &pcbs[i]);
    }
}

pid loader_load(pso_file* f, int pl) {
    if (strcmp((char *)f->signature, "PSO") == 0) {
        // Obtenemos un nuevo PCB
        pcb *pcb;
        POP(&free_pcbs, &pcb);

        /*
         * Cargamos los datos en el PCB
         */

        pcb->pd = (void *)mm_dir_new();

        pcb->kernel_stack = mm_mem_kalloc();
        pcb->kernel_stack_limit = pcb->kernel_stack + PAGE_SIZE;
        pcb->kernel_stack_pointer = pcb->kernel_stack_limit;

        // Escribimos el estado inicial
        pcb->kernel_stack_pointer -= sizeof(task_state_t);
        task_state_t *st = (task_state_t *)pcb->kernel_stack_pointer;
        initialize_task_state(st, (void *)f->_main,
            (void *)(USER_STACK + PAGE_SIZE), pl);

        /*
         * Armamos el mapa de memoria de la tarea
         */

        void *mem_start = ALIGN_TO_PAGE((void *)f->mem_start, FALSE);
        void *mem_end = ALIGN_TO_PAGE((void *)f->mem_end, TRUE);
        uint32_t task_pages = ((uint32_t)(mem_end - mem_start))/PAGE_SIZE;

        int i;
        for (i = 0; i < task_pages; i++)
            new_user_page(pcb->pd, mem_start + i*PAGE_SIZE);

        // Copiamos datos y codigo inicializados
        memcpy(mem_start, (void *)f, f->mem_end_disk - f->mem_start);
        memset((void *)f->mem_end_disk, 0, f->mem_end - f->mem_end_disk);

        APPEND(&pcbs_list, pcb);

        sched_load(pcb - pcbs);
    }

	return 0;
}

void loader_enqueue(int* cola) {
}

void loader_unqueue(int* cola) {
}

void loader_exit(void) {
}

void loader_switchto(pid pd) {
    
}


/* Inicializa un estado de tarea con los siguientes valores iniciales:
 * - los registros de proposito general en 0;
 * - los registros de segmento tienen como valor un selector que
 *   referencia los segmentos de usuario en la GDT;
 * - los flags adquieren el valor SCHED_COMMON_EFLAGS;
 * - el instruction pointer contiene el valor del punto de entrada
 *   ``entry_point``;
 * - el stack pointer contiene el valor de ``stack_pointer``.
 *
 * Tener en cuenta que tanto ``entry_point`` como ``stack_pointer`` deben
 * ser direcciones en el espacio de direcciones virtual de la tarea.
 */
static void initialize_task_state(task_state_t *st, void *entry_point,
    void *stack_pointer, int pl) {

    // Registros de proposito general en cero
    st->eax = NULL;
    st->ecx = NULL;
    st->edx = NULL;
    st->ebx = NULL;
    st->ebp = NULL;
    st->esi = NULL;
    st->edi = NULL;

    // Registros de segmento
    uint32_t ds = (pl == 0) ? GDT_INDEX_KERNEL_DS : GDT_INDEX_USER_DS;
    uint32_t cs = (pl == 0) ? GDT_INDEX_KERNEL_CS : GDT_INDEX_USER_CS;

    st->ds = GDT_SEGSEL(pl, ds);
    st->es = GDT_SEGSEL(pl, ds);
    st->fs = GDT_SEGSEL(pl, ds);
    st->gs = GDT_SEGSEL(pl, ds);
    st->cs = GDT_SEGSEL(pl, cs);

    // Flags
    st->eflags = COMMON_EFLAGS;

    st->eip = (uint32_t)entry_point;

    st->org_esp = (uint32_t)stack_pointer;
    st->org_ss = GDT_SEGSEL(pl, ds);
}
