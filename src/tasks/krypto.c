#include <user/syscalls.h>
#include <user/io.h>
#include <fs.h>
#include <utils_common.h>

#define PAGE_SIZE 4096

#define IN_PATH "/disk/kernel.bin"
#define OUT_PATH "/serial0"

void reader(int to_encrypt) {
    char *buf = palloc();
    int fd = open(IN_PATH, FS_OPEN_RDWR);
    int n;
    while ((n = read(fd, buf, PAGE_SIZE)))
        write(to_encrypt, buf, n);
}

void encrypt(int from_reader, int to_writer) {
    char *key_buf = "soyunaclaveredificildedescifrar!";
    uint32_t key_buf_len = strlen(key_buf);
    int key_buf_idx = 0;

    char *buf = palloc();
    int n;
    while ((n = read(from_reader, buf, PAGE_SIZE))) {
        int i;
        for (i = 0; i < n; i++, key_buf_idx = (key_buf_idx + 1) % key_buf_len)
            buf[i] ^= key_buf[key_buf_idx];

        write(to_writer, buf, n);
    }
}

void writer(int from_encrypt) {
    char *buf = palloc();
    int fd = open(OUT_PATH, FS_OPEN_RDWR);
    int n;
    while ((n = read(from_encrypt, buf, PAGE_SIZE)))
        write(fd, buf, n);
}


int main(void) {

    int pipe_re[2];
    pipe(pipe_re);

    int reader_pid = fork();
    if (reader_pid == 0) {
        // Reader
        close(pipe_re[0]);

        reader(pipe_re[1]);
    }
    else {
        close(pipe_re[1]);

        int pipe_ew[2];
        pipe(pipe_ew);

        int encrypt_pid = fork();
        if (encrypt_pid == 0) {
            // Encrypt
            close(pipe_ew[0]);

            encrypt(pipe_re[0], pipe_ew[1]);
        }
        else {
            // Writer
            close(pipe_re[0]);
            close(pipe_ew[1]);

            writer(pipe_ew[0]);
        }
    }
    return 0;
}
