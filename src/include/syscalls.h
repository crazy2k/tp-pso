#ifndef __SYSCALLS_H__
#define __SYSCALLS_H__

#include <tipos.h>
#include <fs.h>


#define SYSCALLS_NUM_EXIT 1
#define SYSCALLS_NUM_GETPID 2
#define SYSCALLS_NUM_PALLOC 3
#define SYSCALLS_NUM_READ 4
#define SYSCALLS_NUM_WRITE 5
#define SYSCALLS_NUM_SEEK 6
#define SYSCALLS_NUM_CLOSE 7
#define SYSCALLS_NUM_OPEN 8
#define SYSCALLS_NUM_CON_CTL 9
#define SYSCALLS_NUM_RUN 10
#define SYSCALLS_NUM_PIPE 11
#define SYSCALLS_NUM_FORK 12
#define SYSCALLS_NUM_SHARE_PAGE 13
#define SYSCALLS_NUM_STAT 14

#define SYSCALLS_INTERRUPT 0x30ul


#define CON_CTL_CLS_SCREEN 1
#define CON_CTL_DELETE_CUR_CHAR 2
#define CON_CTL_BACKSPACE 3


#ifdef __KERNEL__
// Sólo se compila en modo "kernel"
    #include "isr.h"

    void sys_exit();
    uint32_t sys_getpid();
    void *sys_palloc();
    int sys_read(int fd, void *buf, uint32_t size);
    int sys_write(int fd, const void *buf, uint32_t size);
    int sys_seek(int fd, uint32_t size);
    int sys_close(int fd);
    int sys_open(char* path, uint32_t mode);
    int sys_pipe(int fds[2]);
    int sys_run(const char* path);
    int sys_fork(task_state_t* st);
    int sys_share_page(void *page);


#else
// __TAREA___

// Sólo se compila en modo "tarea"

// Declarar los "wrapers" para los syscalls que incluyen las tareas.


    uint32_t syscall(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx,
        uint32_t esi, uint32_t edi);

    void exit();
    uint32_t getpid();
    void* palloc();
    int read(int fd, void *buf, uint32_t size);
    int write(int fd, const void *buf, uint32_t size);
    int seek(int fd, uint32_t size);
    int close(int fd);
    int open(const char* path, uint32_t mode);
    int stat(const char *path, stat_t *buf);
    int pipe(int fds[2]);
    void con_ctl(uint32_t con, uint32_t oper);
    int run(const char* path);
    int fork();
    int share_page(void *page);


#endif

#endif
