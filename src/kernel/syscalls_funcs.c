#include <loader.h>
#include <sched.h>

void sys_exit() {
    loader_exit();
}

uint32_t sys_getpid() {
    return sched_get_current_pid();
}

void *sys_palloc() {
    return mm_mem_alloc();
}
