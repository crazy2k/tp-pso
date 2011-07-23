#include <fs.h>
#include <device.h>
#include <debug.h>

#include <sched.h>
#include <errors.h>

// Discos
#include <fdd.h>
#include <hdd.h>

// Sistemas de archivos
#include <fat12.h>
#include <fat16.h>
#include <ext2.h>

// Dispositivos
#include <serial.h>
#include <con.h>
#include <fs.h>
#include <utils.h>


#define CONSOLE_PATH "/console"

/*
 * Disco 1;
 */ 
ext2 disk;

/*
 * Se pueden agregar más discos así:
 */
 //~ fat16 disk2; 
 //~ fat32 disk3;
 //~ ext2 disk4;
 //~ resiserfs disk5;
 //~ ntfs disk6;

void fs_init(void) {
    ext2_create(&disk, hdd_open(0));
}

chardev *fs_open(const char *filename, uint32_t flags) {
    // Chequeamos si el archivo esta presente y se abre minimamente para lectura o escritura
	if (filename && (flags & FS_OPEN_RDWR) == 0)
        return NULL;

	if (!strcmp(filename, "/serial0")) return serial_open(0);
	if (!strcmp(filename, "/serial1")) return serial_open(1);
	if (strncmp(filename, CONSOLE_PATH, strlen(CONSOLE_PATH)) == 0) {
        if (strlen(filename) == strlen(CONSOLE_PATH))
            return con_open(NEW_CONSOLE, flags);
        else if (isnumeric(filename + strlen(CONSOLE_PATH))) {
            int num = strtoi(filename + strlen(CONSOLE_PATH));
            return con_open(num, flags);
        } else
            return NULL;
    }        

	if (!strncmp(filename, "/disk/", 6))
        return ext2_open(&disk, filename, flags);
	
	return NULL;
}

int fs_stat(const char *filename, stat_t* st) {
    if (!strncmp(filename, "/disk/", 6))
        return ext2_stat(&disk, filename, st);
    else
        return -ENOFILE;
}

// Syscalls:
// int open(const char* filename, uint_32 flags);



