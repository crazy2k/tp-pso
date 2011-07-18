#include <user/syscalls.h>
#include <user/io.h>
#include <fs.h>

int main(void) {

    __asm__ __volatile__("xchg %bx, %bx");

    return 0;
}
