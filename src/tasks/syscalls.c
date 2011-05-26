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
    return syscall(SYSCALLS_NUM_PALLOC, 0, 0, 0, 0, 0);
}

void exit(void) {
    syscall(SYSCALLS_NUM_EXIT, 0, 0, 0, 0, 0);
}

int getpid(void) {
    return syscall(SYSCALLS_NUM_GETPID, 0, 0, 0, 0, 0);
}
