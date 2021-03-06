#include <user/syscalls.h>
#include <user/utils.h>
#include <user/io.h>
#include <fs.h>
#include <errors.h>

#define SHELL_BUF_SIZE 1024
#define PAGE_SIZE 4096
#define CAT_BUF_SIZE SHELL_BUF_SIZE
#define PROMTP "shell> "

#define CMD_ECHO    "echo"
#define CMD_GETPID  "getpid"
#define CMD_EXIT    "exit"
#define CMD_HELP    "help"
#define CMD_CAT     "cat"

char *shell_commands[] = { CMD_ECHO, CMD_GETPID, CMD_EXIT, CMD_HELP, CMD_CAT };

char shell_buf[SHELL_BUF_SIZE] = { 0 };
char cat_buf[CAT_BUF_SIZE + 1] = { 0 };
//int *pages[1536];
char *bufs[1500];


static char *skip_spaces(char* str);
static char *get_word(char** str);
static void print_help(int fd);
static void cat_file(int console, char* file_name);
static void cat(int console, char* files);
static void specialcat_file(int console, char* file_name);
static void specialcat(int console, char* files);
static void multiread(int console, char *files);
static void hogmem(int console, char *files);
static void swaptest(int console, char *files);


int main(void) {
//    __asm __volatile("xchg %%bx, %%bx" : :);

    char *rest, *command;
    int con = open("/console", FS_OPEN_RDWR);
    int res;

    while (TRUE) {
//        __asm __volatile("xchg %%bx, %%bx" : :);

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
            } else if (strcmp(command, "sc") == 0) {
                specialcat(con, rest);
            } else if (strcmp(command, "mr") == 0) {
                multiread(con, rest);
            } else if (strcmp(command, "hm") == 0) {
                hogmem(con, rest);
//            } else if (strcmp(command, "st") == 0) {
//                swaptest(con, rest);
            } else if (strcmp(command, CMD_HELP) == 0) {
                print_help(con);
            } else if (strlen(command)) {
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

static void multiread_file(int console, char* file_name) {
    printf(console, "starting tasks\n");
    int pid = fork();
    if (pid == 0) {
        printf(console, "starting task 1\n");
        int fd = open(file_name, FS_OPEN_WRONLY);
        char c;
        read(fd, &c, 1);
        printf(console, "data read: %x\n", c);
        close(fd);
        exit();
    } else {
        printf(console, "starting task 2\n");
        int fd = open(file_name, FS_OPEN_WRONLY);
        seek(fd, 1024);
        char c;
        read(fd, &c, 1);
        printf(console, "data read: %x\n", c);
        close(fd);
    }
}

static void hogmem(int console, char *files) {
    char *s = "holitas";
    for (int i = 0; i < 1000; i++) {
//        __asm __volatile("xchg %%bx, %%bx" : :);
        bufs[i] = palloc();
        bufs[i][0] = s[i % 7];
        printf(console, "%d - Set: %x -> %x\n", i, bufs[i], bufs[i][0]);
    }

    for (int i = 0; i < 1000; i++) {
//        __asm __volatile("xchg %%bx, %%bx" : :);
        char expected = s[i % 7];
        char found = bufs[i][0];
        if (expected != found) {
            printf(console, "%x - Expected: %x; Found %x\n", bufs[i], s[i % 7], bufs[i][0]);
        }
    }
    
    write_str(console, "Done\n");
}

static void swaptest_file(int console, char* file_name) {
//    char filename_buf[40];
//    char num_buf[20];
//    int fds[262];
//    for (int i = 0; i < 262; i++) {
//        strcpy(filename_buf, file_name);
//        itostr(i, num_buf);
//        strcat(filename_buf, num_buf);
//        fds[i] = open(file_name, FS_OPEN_WRONLY);
//    }
//    
//    int sum = 0;
//    int i = 0;
//    printf(console, "Num is %s\n", num_buf);

//    int fd = open(file_name, FS_OPEN_WRONLY);
//    if (fd >= 0) {
//        printf(console, "Empezando a leer\n");
//        for (int i = 0; i < 262; i++) {
//            printf(console, "Pagina %d\n", i);
//            pages[i] = palloc();
//            seek(fd, i*PAGE_SIZE);
//            read(fd, pages[i], PAGE_SIZE);
//            printf(console, "v = %d\n", pages[i][0]);
//            sum += pages[i][0];
//        }
//        close(fd);
//        printf(console, "Sum is %d\n", sum);
//    } else
//        printf(console, "No existe el archivo %s\n", file_name);
}

//static void swaptest(int console, char *files) {
//    char *file_name;
//
//    while(strlen(files) && (file_name = get_word(&files))) {
//        swaptest_file(console, file_name);
//
//        files = skip_spaces(files);
//    }
//
//    write_str(console, "\n");
//}

static void multiread(int console, char *files) {
    char *file_name;

    while(strlen(files) && (file_name = get_word(&files))) {
        multiread_file(console, file_name);

        files = skip_spaces(files);
    }

    write_str(console, "\n");
    
}

static void specialcat(int console, char* files) {
    char *file_name;

    while(strlen(files) && (file_name = get_word(&files))) {
        specialcat_file(console, file_name);

        files = skip_spaces(files);
    }

    write_str(console, "\n");
}

static void specialcat_file(int console, char* file_name) {
    int fd = open(file_name, FS_OPEN_WRONLY);
    int chars;

    if (fd >= 0) {
        char block_signature = 0;
        int block = 0;
        int step = 50;

        // cat_buf is one block
        while ((chars = read(fd, cat_buf, CAT_BUF_SIZE))) {
            if (cat_buf[0] != block_signature) {
                printf(console, "Mismatch in block %x. Found %x\n", block, cat_buf[0]);
                break;
            }
            printf(console, "Pass: %d\n", block);

            block_signature += step;
            block += step;
            seek(fd, block*1024);
        }

        close(fd);
    } else
        printf(console, "No existe el archivo %s", file_name);
}

static void specialcat2_file(int console, char* file_name) {
    int fd = open(file_name, FS_OPEN_WRONLY);
    int chars;

    if (fd >= 0) {
        seek(fd, 3*1024);
        // 1KB
        char *buf = "1234567890qwertyuiopasdfghjklzxcvbnm1234567890qwertyuiopasdfghjklzxcvbnm1234567890qwertyuiopasdfghjklzxcvbnm1234567890qwertyuiopasdfghjklzxcvbnm1234567890qwertyuiopasdfghjklzxcvbnm1234567890qwertyuiopasdfghjklzxcvbnm1234567890qwertyuiopasdfghjklzxcvbnm1234567890qwertyuiopasdfghjklzxcvbnm1234567890qwertyuiopasdfghjklzxcvbnm1234567890qwertyuiopasdfghjklzxcvbnm1234567890qwertyuiopasdfghjklzxcvbnm1234567890qwertyuiopasdfghjklzxcvbnm1234567890qwertyuiopasdfghjklzxcvbnm1234567890qwertyuiopasdfghjklzxcvbnm1234567890qwertyuiopasdfghjklzxcvbnm1234567890qwertyuiopasdfghjklzxcvbnm1234567890qwertyuiopasdfghjklzxcvbnm1234567890qwertyuiopasdfghjklzxcvbnm1234567890qwertyuiopasdfghjklzxcvbnm1234567890qwertyuiopasdfghjklzxcvbnm1234567890qwertyuiopasdfghjklzxcvbnm1234567890qwertyuiopasdfghjklzxcvbnm1234567890qwertyuiopasdfghjklzxcvbnm1234567890qwertyuiopasdfghjklzxcvbnm1234567890qwertyuiopasdfghjklzxcvbnm1234567890qwertyuiopasdfghjklzxcvbnm1234567890qwertyuiopasdfghjklzxcvbnm1234567890qwertyuiopasdfghjklzxcvbnm1234567890qwertyuiopasdfghjklzxcvbnm1234567890qwerty";
        write(fd, buf, 1024);
//        
//        char block_signature = 0;
//        int block = 0;
//        int step = 50;
//
//        // cat_buf is one block
//        while ((chars = read(fd, cat_buf, CAT_BUF_SIZE))) {
//            if (cat_buf[0] != block_signature) {
//                printf(console, "Mismatch in block %x. Found %x\n", block, cat_buf[0]);
//                break;
//            }
//            printf(console, "Pass: %d\n", block);
//
//            block_signature += step;
//            block += step;
//            seek(fd, block*1024);
//        }

        close(fd);

        fd = open(file_name, FS_OPEN_RDONLY);
        seek(fd, 3*1024);
        chars = read(fd, cat_buf, CAT_BUF_SIZE);
        write(console, cat_buf, chars);
    } else
        printf(console, "No existe el archivo %s", file_name);
}

static void cat_file(int console, char* file_name) {
    int fd = open(file_name, FS_OPEN_RDONLY);
    int chars;

    if (fd >= 0) {
        while ((chars = read(fd, cat_buf, CAT_BUF_SIZE)))
            write(console, cat_buf, chars);

        close(fd);
    } else
        printf(console, "No existe el archivo %s", file_name);
}
