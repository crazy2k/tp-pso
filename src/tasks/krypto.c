#include <user/syscalls.h>
#include <user/io.h>
#include <fs.h>

#define PAGE_SIZE 4096

int main(void) {

/*
    char *buf = palloc();
    int fd = open("/disk/kernel.bin", FS_OPEN_RDONLY);
    int serial_fd = open("/serial0", FS_OPEN_RDWR);
    int chars;
    while (chars = read(fd, buf, PAGE_SIZE))
        write(serial_fd, buf, chars);
*/
    int pipe_re[2];
    pipe(pipe_re);

    int reader_pid = fork();
    if (reader_pid == 0) {
        // Reader
        close(pipe_re[0]);

        char *buf = palloc();
        int fd = open("/disk/kernel.bin", FS_OPEN_RDWR);
        int n;
        while (n = read(fd, buf, PAGE_SIZE))
            write(pipe_re[1], buf, n);
    }
    else {
        close(pipe_re[1]);

        int pipe_ew[2];
        pipe(pipe_ew);

        int encrypt_pid = fork();
        if (encrypt_pid == 0) {
            // Encrypt
            close(pipe_ew[0]);

            char *key_buf = "soyunaclaveredificildedescifrar!";

            char *buf = palloc();
            int n;
            while (n = read(pipe_re[0], buf, PAGE_SIZE)) {
                int i;
                for (i = 0; i < n; i++)
                    buf[i] = buf[i] ^ key_buf[i % strlen(key_buf)];
                    
                write(pipe_ew[1], buf, n);
            }
        }
        else {
            // Writer

            close(pipe_re[0]);
            close(pipe_ew[1]);

            char *buf = palloc();
            int fd = open("/serial0", FS_OPEN_RDWR);
            int n;
            while (n = read(pipe_ew[0], buf, PAGE_SIZE))
                write(fd, buf, n);
        }
    }
    return 0;
}
