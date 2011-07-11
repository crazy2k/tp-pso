#include <user/syscalls.h>
#include <user/utils.h>
#include <user/io.h>
#include <fs.h>
#include <errors.h>

#define SHELL_BUF_SIZE 160
#define PROMTP "shell> "

char shell_buf[SHELL_BUF_SIZE] = { 0 };

static char *skip_spaces(char* str);
static char *get_word(char** str);
static void print_help(int fd);


int main(void) {

    char *rest, *command;
    int con = open("/console", FS_OPEN_RDWR);
    int res;

    while (TRUE) {

        write_str(con, PROMTP);

        res = scanln(con, shell_buf, SHELL_BUF_SIZE);

        if (res > 0) {
            rest = skip_spaces(shell_buf);
            command = get_word(&rest);
            rest = skip_spaces(rest);
            
            if (strcmp(command, "echo") == 0) {
                println(con, rest);
            } else if (strcmp(command, "getpid") == 0) {
                uint32_t pid = getpid();
                write_decimal(con, pid);
                write(con, "\n", 1);
            } else if (strcmp(command, "exit") == 0) {
                exit();
            } else if (strcmp(command, "help") == 0) {
                print_help(con);
            } else {
                if (run(command) > 0)
                    println(con, "Comando desconocido");
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

static void print_help(int fd) {
    println(fd, "Help!!");
}
