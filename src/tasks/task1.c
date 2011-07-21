#include <user/syscalls.h>
#include <user/io.h>
#include <fs.h>

int main(void) {

    int con = open("/console0", FS_OPEN_RDWR);
    println(con, "task1 estuvo aqui");

    int pid = fork();
    if (pid == 0)
        println(con, "soy un hijo");
    else
        println(con, "soy un padre");

    while (1) ;
    return 0;
}
