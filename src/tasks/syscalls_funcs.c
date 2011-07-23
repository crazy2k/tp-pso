#include <syscalls.h>


uint32_t syscall(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx,
    uint32_t esi, uint32_t edi) {
    uint32_t ret;
    asm volatile("int %1\n"
        : "=a" (ret)
        : "i" (SYSCALLS_INTERRUPT),
        "a" (eax),
        "b" (ebx),
        "c" (ecx),
        "d" (edx),
        "S" (esi),
        "D" (edi)
        : "cc", "memory");

    return ret;
}


void* palloc() {
    return (void *)syscall(SYSCALLS_NUM_PALLOC, 0, 0, 0, 0, 0);
}

void exit(void) {
    syscall(SYSCALLS_NUM_EXIT, 0, 0, 0, 0, 0);
}

uint32_t getpid(void) {
    return syscall(SYSCALLS_NUM_GETPID, 0, 0, 0, 0, 0);
}

int read(int fd, void *buf, uint32_t size) {
    return syscall(SYSCALLS_NUM_READ, (uint32_t)fd, (uint32_t)buf,
        (uint32_t)size, 0, 0);
}

int write(int fd, const void *buf, uint32_t size) {
    return syscall(SYSCALLS_NUM_WRITE, (uint32_t)fd, (uint32_t)buf,
        (uint32_t)size, 0, 0);
}

int seek(int fd, uint32_t size) {
    return syscall(SYSCALLS_NUM_SEEK, (uint32_t)fd, (uint32_t)size, 0, 0, 0);
}

int close(int fd) {
    return syscall(SYSCALLS_NUM_CLOSE, (uint32_t)fd, 0, 0, 0, 0);
}

int open(const char* path, uint32_t mode) {
    return syscall(SYSCALLS_NUM_OPEN, (uint32_t)path, mode, 0, 0, 0);
}

int stat(const char* path, stat_t* st) {
    return syscall(SYSCALLS_NUM_STAT, (uint32_t)path, (uint32_t)st, 0, 0, 0);
}

void con_ctl(uint32_t con, uint32_t oper) {
    syscall(SYSCALLS_NUM_CON_CTL, con, oper, 0, 0, 0);
}

int run(const char* path) {
    return syscall(SYSCALLS_NUM_RUN, (uint32_t)path, 0, 0, 0, 0);
}

int pipe(int fds[2]) {
    return syscall(SYSCALLS_NUM_PIPE, (uint32_t)fds, 0, 0, 0, 0);
}

int fork() {
    return syscall(SYSCALLS_NUM_FORK, 0, 0, 0, 0, 0);
}


int share_page(void* page) {
    return syscall(SYSCALLS_NUM_SHARE_PAGE, (uint32_t)page, 0, 0, 0, 0);
}
