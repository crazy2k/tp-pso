#include <user/syscalls.h>
#include <user/utils.h>
#include <user/io.h>
#include <fs.h>

#define SHELL_BUF_SIZE 160
#define PROMTP "shell> "


static char *skip_spaces(char* str);
static char *get_word(char** str);


int main(void) {

    int con = open("/console0", FS_OPEN_RDWR);
    int res;

    char shell_buf[SHELL_BUF_SIZE] = { 0 };
    char *rest, *command;

    while (TRUE) {

        write_str(con, PROMTP);

        res = scanln(con, shell_buf, SHELL_BUF_SIZE);

        if (res > 0) {
            rest = skip_spaces(shell_buf);
            command = get_word(&rest);
            rest = skip_spaces(rest);
            
            if (strcmp(command,"echo") == 0) {
                write_str(con, rest);
            } else if (strcmp(command,"help") == 0) {
            
            } else { 
                write_str("Comando desconocido", rest);
            }
        }
    }

    return 0;
}

// Se saltea todos los espacios y retorna el primer elemento que no es un espacio
static char *skip_spaces(char* str) {
    if (str)
        while (*str == ' ') str++;
 
    return str;
}

/* Deja apuntando en el resultado la siguiente palabra q encuentra, 
   pone un NULL al final de la misma y deja en str la posicion siguente 
   a donde dejo el NULL*/
static char *get_word(char** str) {
    char *result = *str,
        *curr = *str;

    while (*curr != NULL && *curr != ' ')
        curr++;

    if (*curr != NULL ) {
        *curr = NULL;
        ++curr;
    }
    *str = curr;

    return result;
}

