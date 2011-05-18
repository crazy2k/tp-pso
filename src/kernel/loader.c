#include <loader.h>

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
} tss_t;

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
