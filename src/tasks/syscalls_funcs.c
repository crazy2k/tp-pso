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

int opencon() {
    return syscall(SYSCALLS_NUM_OPENCON, 0, 0, 0, 0, 0);
}

void nextcon() {
    syscall(SYSCALLS_NUM_NEXTCON, 0, 0, 0, 0, 0);
}
