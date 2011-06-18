#include <user/syscalls.h>
#include <user/io.h>
#include <fs.h>

int main(void) {

   // __asm__ __volatile__("xchg %bx, %bx");
    
    int con = open("/console0", FS_OPEN_RDWR);
    int res = 0;

    write(con, "shell>", 6);
    write(con, " ", 1);

    char buff[80] = { 0 };
//    char chr;

    while (1) {

        res = scanln(con, buff, 80);

//        __asm__ __volatile__("xchg %bx, %bx");

        if (res > 0)
            println(con, buff, res);



/*
       res = read(con, &chr, 1);

       if (chr)
            write_line(con, &chr, 1);
*/

    }

    return 0;
}
