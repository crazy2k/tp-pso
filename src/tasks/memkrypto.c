#include <user/syscalls.h>
#include <user/io.h>
#include <fs.h>
#include <utils_common.h>

#define PAGE_SIZE 4096

#define IN_PATH "/disk/kernel.bin"
#define OUT_PATH "/serial0"

#define BUFFERS 4

#define WAIT(p) ({ \
        char __c; \
        read(p, &__c, 1); \
    })
#define SIGNAL(p) ({ \
        char __c = 1; \
        write(p, &__c, 1); \
    })

char *buffers[4];


void reader(int from_writer, int to_encrypt) {
    int fd = open(IN_PATH, FS_OPEN_RDWR);

    int bufcounter = 0;
    char *buf = palloc();
    int n;
    while ((n = read(fd, buf, PAGE_SIZE)) > 0) {
        WAIT(from_writer);
        memcpy(buffers[bufcounter], buf, n);
        SIGNAL(to_encrypt);

        bufcounter = (bufcounter + 1) % BUFFERS;
    }
}

void encrypt(int from_reader, int to_writer, uint32_t filesize) {
    char *key_buf = "soyunaclaveredificildedescifrar!";
    uint32_t key_buf_len = strlen(key_buf);
    int key_buf_idx = 0;

    int bufcounter = 0;
    while (filesize > 0) {
        int n = (filesize >= PAGE_SIZE) ? PAGE_SIZE : filesize;

        WAIT(from_reader);
        int i;
        for (i = 0; i < n; i++, key_buf_idx = (key_buf_idx + 1) % key_buf_len)
            buffers[bufcounter][i] ^= key_buf[key_buf_idx];
        SIGNAL(to_writer);

        filesize -= n;
        bufcounter = (bufcounter + 1) % BUFFERS;
    }
}

void writer(int from_encrypt, int to_reader, int filesize) {
    int fd = open(OUT_PATH, FS_OPEN_RDWR);

    int bufcounter = 0;
    char *buf = palloc();
    while (filesize > 0) {
        int n = (filesize >= PAGE_SIZE) ? PAGE_SIZE : filesize;

        WAIT(from_encrypt);
        memcpy(buf, buffers[bufcounter], n);
        SIGNAL(to_reader);

        write(fd, buf, n);

        filesize -= n;
        bufcounter = (bufcounter + 1) % BUFFERS;
    }
}

int main(void) {
    int i;
    for (i = 0; i < BUFFERS; i++) {
        buffers[i] = palloc();
        share_page(buffers[i]);
    }

    int pipe_re[2];
    pipe(pipe_re);

    int pipe_wr[2];
    pipe(pipe_wr);

    stat_t st;
    stat(IN_PATH, &st);

    int reader_pid = fork();
    if (reader_pid == 0) {
        // Reader
        close(pipe_re[0]);

        for (i = 0; i < BUFFERS; i++)
            SIGNAL(pipe_wr[1]);
        close(pipe_wr[1]);

        reader(pipe_wr[0], pipe_re[1]);
    }
    else {
        close(pipe_re[1]);
        close(pipe_wr[0]);

        int pipe_ew[2];
        pipe(pipe_ew);

        
        int encrypt_pid = fork();
        if (encrypt_pid == 0) {
            // Encrypt
            close(pipe_ew[0]);

            encrypt(pipe_re[0], pipe_ew[1], st.size);
        }
        else {
            // Writer
            close(pipe_re[0]);
            close(pipe_ew[1]);

            writer(pipe_ew[0], pipe_wr[1], st.size);
        }
    }
    return 0;
}
