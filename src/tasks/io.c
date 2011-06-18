#include <user/io.h>
#include <user/utils.h>

#define READ_BUF_SIZE (80 * 25) 

static char read_buf[READ_BUF_SIZE];


void println(uint32_t fd, char* src, int size) {
    write(fd, src, size);
    write(fd, "\n", 1);
}

int scanln(uint32_t fd, char* dest, int size) {
    int pos = 0; 
    int total = size < READ_BUF_SIZE ? size : READ_BUF_SIZE;    
    uint8_t byte;

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

        }
    } 

    memcpy(dest, read_buf, pos);
    return pos;
}

/*            switch (chr) {
                //carriage return:
                case 13:
                    if (terminal->len < DEV_TERMINAL_BUF_LENGTH) {
                        kputc('\n');
                        terminal->buffer[dev_terminal_next_pos(terminal)] = '\n';
                        terminal->len++;
                    }
                break;
                //caracteres imprimibles
                case 32 ... 126:
                    if (terminal->len < DEV_TERMINAL_BUF_LENGTH - 1) {
                        kputc(chr);
                        terminal->buffer[dev_terminal_next_pos(terminal)] = chr;
                        terminal->len++;
                    }
                break;

                //backspace:
                case 8:
                    if (terminal->len > 0) {
                        cur_pos = get_current_pos();

                        set_current_pos(cur_pos - SCREEN_CHAR_SIZE);
                        kputc(' ');
                        set_current_pos(cur_pos - SCREEN_CHAR_SIZE);
                        terminal->len--;
                    }
                break;

                //tab:
                case 9:
                    if (terminal->len < DEV_TERMINAL_BUF_LENGTH - 4) {
                        kputs("    ");
                        for (int i = 0; i < 4; i++) {                        
                            terminal->buffer[dev_terminal_next_pos(terminal)] = ' ';
                            terminal->len++;
                        }
                    }
                break;

                //delete:
                case 127:

                break;
        }
    }*/
