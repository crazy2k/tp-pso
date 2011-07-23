#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>

#define BUF_SIZE 1024

char *key_buf = "soyunaclaveredificildedescifrar!";
char buf[BUF_SIZE];

int main(int argc, char *argv[]) {
    int fd = open(argv[1], O_RDONLY);
    int n;
    while (n = read(fd, buf, BUF_SIZE)) {
        int i;
        for (i = 0; i < n; i++) {
            buf[i] = buf[i] ^ key_buf[i % strlen(key_buf)];
            write(1, buf + i, 1);
        }
    }
}
