#include <user/syscalls.h>
#include <user/io.h>
#include <user/utils.h>

#define READ_BUF_SIZE (80 * 25) 

static char read_buf[READ_BUF_SIZE];


void println(uint32_t fd, char* src, int size) {
    write(fd, src, size);
    write(fd, "\n", 1);
}

int scanln(uint32_t fd, char* dest, int size) {
    return scanln_autocomp(fd, dest, size, NULL);
}

int scanln_autocomp(uint32_t fd, char* dest, int size, int (*autocompl) (char *read_buf, int pos) ) {
    int pos = 0; 
    int total = size < READ_BUF_SIZE ? size : READ_BUF_SIZE;    
    uint8_t byte;
    int chars, i;

    while (pos < total && (pos == 0 || read_buf[pos-1] != NULL)) {
        read(fd, &byte, 1);

        switch (byte) {
            //caracteres imprimibles
            case 32 ... 126: 
                write(fd, &byte, 1);
                read_buf[pos++] = byte;
            break;

            //carriage return:
            case '\n':
            case '\r':
                write(fd, "\n", 1);
                read_buf[pos++] = NULL;
            break;

            //backspace:
            case 8:
                if (pos > 0) {
                    for (i = 0; i < (read_buf[pos] == '\t' ? 4 : 1); i++)
                        con_ctl(fd, CON_CTL_BACKSPACE);
                    pos--;
                }
            break;

            //delete:
            case 127:
                con_ctl(fd, CON_CTL_DELETE_CUR_CHAR);
            break;

            //tab:
            case 9:
                chars = autocompl ? autocompl(read_buf, pos) : -1;
                if (chars == -1) { //Fallo el autocompletado, o no estaba disponible
                    write(fd, "\t", 1);
                    read_buf[pos++] = '\t';
                } else 
                    pos+= chars;
            break;

        }
    } 

    memcpy(dest, read_buf, pos);
    return pos;
}
