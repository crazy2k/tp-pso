#ifndef __SYSCALLS_H__
#define __SYSCALLS_H__

#include <tipos.h>


#define SYSCALLS_NUM_EXIT 1
#define SYSCALLS_NUM_GETPID 2
#define SYSCALLS_NUM_PALLOC 3
#define SYSCALLS_INTERRUPT 0x30ul


#ifdef __KERNEL__
// Sólo se compila en modo "kernel"

    void sys_exit();
    uint32_t sys_getpid();
    void* sys_palloc();

#else
// __TAREA___

// Sólo se compila en modo "tarea"

// Declarar los "wrapers" para los syscalls que incluyen las tareas.


    uint32_t syscall(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx,
        uint32_t esi, uint32_t edi);

    void exit();
    uint32_t getpid();
    void* palloc();

#endif

#endif
