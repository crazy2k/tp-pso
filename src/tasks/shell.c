#include <user/syscalls.h>
#include <user/utils.h>
#include <user/io.h>
#include <fs.h>
#include <errors.h>

#define SHELL_BUF_SIZE 160
#define CAT_BUF_SIZE SHELL_BUF_SIZE
#define PROMTP "shell> "

#define CMD_ECHO    "echo"
#define CMD_GETPID  "getpid"
#define CMD_EXIT    "exit"
#define CMD_HELP    "help"
#define CMD_CAT     "cat"

char *shell_commands[] = { CMD_ECHO, CMD_GETPID, CMD_EXIT, CMD_HELP, CMD_CAT };

char shell_buf[SHELL_BUF_SIZE] = { 0 };
char cat_buf[CAT_BUF_SIZE] = { 0 };

static char *skip_spaces(char* str);
static char *get_word(char** str);
static void print_help(int fd);
static void cat_file(int console, char* file_name);
static void cat(int console, char* files);


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
            
            if (strcmp(command, CMD_ECHO) == 0) {
                println(con, rest);
            } else if (strcmp(command, CMD_GETPID) == 0) {
                printf(con, "%d\n", getpid());
            } else if (strcmp(command, CMD_EXIT) == 0) {
                exit();
            } else if (strcmp(command, CMD_CAT) == 0) {
                cat(con, rest);
            } else if (strcmp(command, CMD_HELP) == 0) {
                print_help(con);
            } else {
                if (run(command) < 0)
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
    println(fd, "Comandos disponibles:");

    int i;
    for (i = 0; i < sizeof(shell_commands)/sizeof(char *); i++)
        printf(fd, "\t%s\n", shell_commands[i]);
}

static void cat(int console, char* files) {
    char *file_name;

    while(strlen(files) && (file_name = get_word(&files))) {
        cat_file(console, file_name);

        files = skip_spaces(files);
    }

    write_str(console, "\n");
}

static void cat_file(int console, char* file_name) {
    int fd = open(file_name, FS_OPEN_RDONLY);
    int chars;

    if (fd >= 0) {
        do {
            chars = read(fd, cat_buf, CAT_BUF_SIZE);

            write(console, cat_buf, chars);
        } while (chars == CAT_BUF_SIZE);
        close(fd);
    } else
        printf(console, "No existe el archivo %s", file_name);
}
