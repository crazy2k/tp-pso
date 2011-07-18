#include <user/syscalls.h>
#include <user/io.h>
#include <fs.h>

int main(void) {

    int con = open("/console0", FS_OPEN_RDWR);
    println(con, "task1 estuvo aqui");

    return 0;
}
