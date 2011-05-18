#include <idt.h>
#include <sched.h>
#include <loader.h>
#include <utils.h>
#include <i386.h>

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
    memset(tasks, 0, sizeof(tasks));

    free_tasks = NULL;
    int i;
    for (i = 0; i < MAX_PID; i++) {
        APPEND(&free_tasks, &tasks[i]);
    }
    breakpoint();
}


void sched_load(pid pd) {
    sched_task *task;
    POP(&free_tasks, &task);

    // Llenar task
    task->blocked = FALSE;
    task->quantum = DEFAULT_QUANTUM;
    task->rem_quantum = task->quantum;

    APPEND(&task_list, task);
}

void sched_unblock(pid pd) {
    sched_task *task = current_task();
    task->blocked = FALSE;
}

int sched_exit() {
    sched_task *task = current_task();

    UNLINK_NODE(&task_list, task);
    APPEND(&free_tasks, task);

    return get_pid(next_executable_task(task));
}

int sched_block() {
    sched_task *task = current_task();
    task->blocked = TRUE;
	return get_pid(task->next);
}

int sched_tick() {
    sched_task *current = current_task();
    current->rem_quantum--;

    if (!current->rem_quantum) {
        restart_quantum(current);

        sched_task *next_task = next_executable_task(current);
        if (next_task)
            return get_pid(next_task);
    }

    // Si la tarea actual aun tiene quantum o no encontramos otra tarea a
    // ejecutar, seguimos con la misma
	return get_pid(current);
}

static sched_task *next_executable_task(sched_task *task) {
    sched_task *candidate = task->next;
    while (candidate != task) {
        if (!(candidate->blocked))
            return candidate;

        candidate = candidate->next;
    }
    // Nunca deberiamos llegar aqui si hay siempre una tarea idle
    return NULL;
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
