#include <user/syscalls.h>
#include <user/con.h>

int main(void) {

   // __asm__ __volatile__("xchg %bx, %bx");

    
    int con = open_console();

    write(con, "chau", 4);
    write(con, " ", 1);

    char buff[20] = { 0 };

    while (1) {
        read(con, buff, 1);

        if (buff[0] & 0x7F)
            write(con, buff, 1);

    }

    return;
}
