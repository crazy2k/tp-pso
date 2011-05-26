#ifndef __SYSCALLS_H__
#define __SYSCALLS_H__

#include <tipos.h>

#ifdef __KERNEL__

// Sólo se compila en modo "kernel"

#define SYSCALLS_NUM_EXIT 1
#define SYSCALLS_NUM_GETPID 2
#define SYSCALLS_NUM_PALLOC 3

void sys_exit();
uint32_t sys_getpid();
void* sys_palloc();

#else
// __TAREA___

// Sólo se compila en modo "tarea"

// Declarar los "wrapers" para los syscalls que incluyen las tareas.

#endif

#endif
