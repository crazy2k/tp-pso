#include <user/con.h>

#define READ_BUFF_SIZE (80 * 25) 

static char console_read_buff[READ_BUFF_SIZE];


void con_print(uint32_t console, char* src, int size) {

    write(console, src, size);

}

int con_scan(uint32_t console, char* dest, int size) {
    int result = 0, i, chars;
    char chr;

    while (TRUE) { 
        chars = read(console, console_read_buff, READ_BUFF_SIZE);

        for (i = 0; < chars; i++) {
            chr = console_read_buff[i]

            switch (chr) {
                //carriage return:
                case '\r':
                    dest[result++] = 'current';
                    return result;
                break;
                //caracteres imprimibles
                case 32 ... 126: 
                    dest[result++] = 'current';
                break;

            }
        }
    }
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
