#ifndef __SYSCALLS_H__
#define __SYSCALLS_H__

#include <tipos.h>

#ifdef __KERNEL__

// Sólo se compila en modo "kernel"

#define SYSCALLS_NUM_EXIT 1

void sys_exit();

#else
// __TAREA___

// Sólo se compila en modo "tarea"

// Declarar los "wrapers" para los syscalls que incluyen las tareas.

#endif

#endif
