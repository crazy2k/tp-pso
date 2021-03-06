#include <idt.h>
#include <sched.h>
#include <loader.h>
#include <utils.h>
#include <i386.h>
#include <debug.h>

#define DEFAULT_QUANTUM 20


typedef struct sched_task sched_task;
struct sched_task {
    bool blocked;
    uint32_t quantum;
    uint32_t rem_quantum;

    sched_task *next;
    sched_task *prev;
};

static sched_task *next_executable_task(sched_task *task);
static sched_task *current_task();
static int get_pid(sched_task *task);
static void restart_quantum(sched_task *task);


static sched_task tasks[MAX_PID];

// Lista de tareas del scheduler
static sched_task *task_list;
// Lista de tareas libres
static sched_task *free_tasks;

void sched_init(void) {
    CREATE_FREE_OBJS_LIST(free_tasks, tasks, MAX_PID);
}

void sched_load(pid pd) {
    debug_printf("sched_load: Loading %d\n", pd);
    sched_task *task = POP(&free_tasks);

    // Llenar task
    task->blocked = FALSE;
    task->quantum = DEFAULT_QUANTUM;
    task->rem_quantum = task->quantum;

    APPEND(&task_list, task);
}

void sched_unblock(pid pd) {
    debug_printf("sched_unblock: Unblocking %d\n", pd);
    tasks[pd].blocked = FALSE;
}

int sched_exit() {
    sched_task *task = current_task();

    int next_pid = get_pid(next_executable_task(task));

    UNLINK_NODE(&task_list, task);
    APPEND(&free_tasks, task);

    return next_pid;
}

int sched_block() {
    debug_printf("sched_block: before\n");
    sched_task *task = current_task();
    sched_task *next_task = task_list = next_executable_task(task);

    task->blocked = TRUE;
    debug_printf("sched_block: task blocked\n");

    return get_pid(next_task);
}

int sched_tick() {
    sched_task *current = current_task();

//    debug_printf("sched_tick: running before is %x, blocked: %x\n", get_pid(current), current->blocked);

    //vga_printf(0, 0, "pid = %x, rem_quantum = %x", 0x0F, sched_get_current_pid(),
    //    current_task()->rem_quantum);

    if (--(current->rem_quantum) == 0) {
        restart_quantum(current);
        task_list = current = next_executable_task(current);
    }

//    debug_printf("sched_tick: running after is %x\n", get_pid(current));

    // Si la tarea actual aun tiene quantum o no encontramos otra tarea a
    // ejecutar, seguimos con la misma
    return get_pid(current);
}

int sched_get_current_pid() {
    return get_pid(current_task());
}

static sched_task *next_executable_task(sched_task *task) {
    sched_task *candidate = task->next;
    //Siempre existe un candidate sin bloquear (debido a la presencia de idle)
    while (candidate->blocked) {
//        debug_printf("next_executable_task: cycle: current is %d, and next is %d\n", get_pid(candidate), get_pid(candidate->next));
        candidate = candidate->next;
    }

/*  debug_printf("Proceso candidato #%x seleccionado esta ", get_pid(candidate));
    debug_printf(candidate->blocked ? "Bloqueado\n" : "Desbloqueado\n");*/

    return candidate;
}

static void restart_quantum(sched_task *task) {
    task->rem_quantum = task->quantum;
}

static int get_pid(sched_task *task) {
    return (task - tasks);
}

static sched_task *current_task() {
    return task_list;
}
