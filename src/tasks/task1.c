#include <user/syscalls.h>
#include <user/io.h>

int main(void) {

   // __asm__ __volatile__("xchg %bx, %bx");
    
    int con = open_console();
    int res = 0;

    write(con, "chau", 4);
    write(con, " ", 1);

    char buff[20] = { 0 };
    //char chr;

    while (1) {

        res = read_line(con, buff, 80);

        __asm__ __volatile__("xchg %bx, %bx");

        if (res > 0)
            write_line(con, buff, res);


/*
       read(con, &chr, 1);

       if (buff[0] & 0x7F)
            write(con, &chr, 1);
*/

    }

    return 0;
}
