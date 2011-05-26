#include  <syscalls.h>

int main(void) {

    __asm__ __volatile__("xchg %bx, %bx");


    return getpid();
}
