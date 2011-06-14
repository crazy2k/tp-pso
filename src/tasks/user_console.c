#include <user/con.h>


void print(uint32_t console, char* src, int size) {

    write(console, src, size);

}

int scan(uint32_t console, char* dest, int size) {

    read(console, src, size);
}
