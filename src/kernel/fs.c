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
    // Chequeamos si el archivo se abre minimamente para lectura o escritura
	if ((flags & FS_OPEN_RDWR) == 0)
        return NULL;

	if (!strcmp(filename, "/serial0")) return serial_open(0);
	if (!strcmp(filename, "/serial1")) return serial_open(1);
	if (strncmp(filename, CONSOLE_PATH, strlen(CONSOLE_PATH)) == 0) {
        int num = strtoi(filename + strlen(CONSOLE_PATH));
        return con_open(num, flags);
    }        

	/*
	 * Pedido para el disco 1: Usamos fat12 para abrirlo
	 */ 
	if (!strcmp(filename, "/disk/")) return ext2_open(&disk, filename, flags);
	
	return NULL;
}

// Syscalls:
// int open(const char* filename, uint_32 flags);



