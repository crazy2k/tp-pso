#include <user/syscalls.h>
#include <user/io.h>
#include <user/utils.h>
#include <stdarg.h>

#define READ_BUF_SIZE (80 * 25) 
#define CONV_BUF_SIZE (16) 

static char read_buf[READ_BUF_SIZE];


void write_hex(uint32_t fd, uint32_t num) {

}

void write_decimal(uint32_t fd, int num) {
    static char buf[CONV_BUF_SIZE];
    
    itostr(num, buf);
    write(fd, buf, strlen(buf));
}

void write_str(uint32_t fd, char* src) {    
    write(fd, src, strlen(src));
}

void println(uint32_t fd, char* src) {
    write_str(fd, src);
    write(fd, "\n", 1);
}

void printf(uint32_t fd, char* format, ...) {
    va_list vargs;
    va_start(vargs, format);

    char c;
    while ((c = *format++) != '\0') {
        if (c != '%')
            write(fd, &c, 1);
        else {
            c = *format++;
            if (c == 'x')
                write_hex(fd, (uint32_t)va_arg(vargs, int));
            else if (c == 'd')
                write_decimal(fd, (int)va_arg(vargs, int));
        }
    }

    va_end(vargs);
}

int scanln(uint32_t fd, char* dest, int size) {
    return scanln_autocomp(fd, dest, size, NULL);
}

int scanln_autocomp(uint32_t fd, char* dest, int size, int (*autocompl) (char *read_buf, int pos) ) {
    int pos = 0;
    int total = min(size, READ_BUF_SIZE);
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
