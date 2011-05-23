#include <loader.h>
#include <isr.h>
#include <mm.h>
#include <utils.h>
#include <i386.h>
#include <sched.h>

#define COMMON_EFLAGS 0x3202
#define USER_STACK 0xC0000000
#define IDLE_MAIN_SIZE 0x00000100

extern void loader_switch_stack_pointers(void **old_stack_top, void
    **new_stack_top);
extern func_main idle_main;

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
static pcb *current_pcb();
static int get_pid(pcb *pcb);


static pcb pcbs[MAX_PID];
static pcb *free_pcbs = NULL;
static pcb *pcbs_list = NULL;
// TSS del sistema. Su valor de esp0 se actualiza en los cambios de contexto.
static tss_t tss_struct;
static tss_t *tss = NULL;

void loader_init(void) {
    memset(pcbs, 0, sizeof(pcbs));

    free_pcbs = NULL;
    int i;
    for (i = 0; i < MAX_PID; i++) {
        APPEND(&free_pcbs, &pcbs[i]);
    }

    pso_file idle_pso_file = {
            .signature = "PSO",
            .mem_start = (uint32_t)idle_main,
            .mem_end_disk = (uint32_t)idle_main + IDLE_MAIN_SIZE,
            .mem_end = (uint32_t)idle_main + IDLE_MAIN_SIZE,
            ._main = idle_main,
        };
    loader_load(&idle_pso_file, 0);
}


pid loader_load(pso_file* f, int pl) {
    if (strcmp((char *)f->signature, "PSO") == 0) {
        // Obtenemos un nuevo PCB
        pcb *pcb = POP(&free_pcbs);

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

        sched_load(get_pid(pcb));
    }

	return 0;
}

void loader_enqueue(int *cola) {
    UNLINK_NODE(&pcbs_list, current_pcb());

    pcb *queue;
    if (*cola == -1) {
        queue = NULL;
        *cola = get_pid(current_pcb());
    }
    else
        queue = &pcbs[*cola];

    APPEND(&queue, current_pcb());

    loader_switchto(sched_block());
}

void loader_unqueue(int *cola) {
    pcb *queue = &pcbs[*cola];
    pcb *pcb = POP(&queue);

    sched_unblock(get_pid(pcb));
}

void loader_exit(void) {
    mm_dir_free(current_pcb()->pd);

    // TODO: Completar


}

void loader_switchto(pid pd) {
    if (get_pid(current_pcb()) == pd)
        return;

    uint32_t eflags = disable_interrupts();

    pcb *old_pcb = current_pcb();
    pcb *new_pcb = pcbs_list = &pcbs[pd];

    lcr3(new_pcb->pd);
    setup_tss(new_pcb->kernel_stack_limit);

    loader_switch_stack_pointers(&old_pcb->kernel_stack_pointer,
        &new_pcb->kernel_stack_pointer);

    restore_eflags(eflags);
    
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

static pcb *current_pcb() {
    return pcbs_list;
}

static int get_pid(pcb *pcb) {
    return (pcb - pcbs);
}

/* Si no existe la TSS, crea una y la configura con los valores para el
 * segmento y offset del stack de nivel 0, carga un descriptor para ella en la
 * GDT y carga el Task Register con un selector que la referencia.
 *
 * Si ya existe, simplemente actualiza el valor del offset del stack de nivel
 * 0.
 *
 * - ``kernel_stack`` es la direccion del stack pointer para el stack de nivel
 *   0 de la tarea en su espacio de direcciones
 */
void setup_tss(uint32_t kernel_stack) {
    if (tss) {
        tss->esp0 = kernel_stack;
        return;
    }

    tss = &tss_struct;
    memset(tss, 0, sizeof(tss_t));

    // Solo nos interesa el segmento de stack para el kernel
    tss->ss0 = GDT_SEGSEL(0x0, GDT_INDEX_KERNEL_DS);
    tss->esp0 = kernel_stack;

    // Escribimos el descriptor de la TSS en la GDT
    gdt[GDT_INDEX_TSS] = GDT_DESC_BASE((uint32_t)tss) |
        GDT_DESC_LIMIT(sizeof(tss_t)) | GDT_DESC_DPL(0x0) |
        GDT_DESC_TYPE(GDT_F_32BTA) | GDT_DESC_G | GDT_DESC_P;

    // Cargamos el Task Register con un selector para la TSS
    uint16_t segsel = GDT_SEGSEL(0x0, GDT_INDEX_TSS);
    ltr(segsel);
}

