#include  <syscalls.h>

int main(void) {

    int con = open_console();

    char buff[100] = { 0 };
    read(con, buff, 1);

    write(con, buff, 1);

    __asm__ __volatile__("xchg %bx, %bx");

    while (1);

    return;
}
