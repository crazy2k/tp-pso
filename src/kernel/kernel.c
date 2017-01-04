#include <pic.h>
#include <idt.h>
#include <vga.h>
#include <mm.h>
#include <swap.h>
#include <sched.h>
#include <gdt.h>
#include <debug.h>
#include <loader.h>
#include <syscalls.h>
#include <i386.h>
#include <con.h>
#include <serial.h>
#include <hdd.h>
#include <fs.h>
#include <ext2.h>
#include <pipe.h>

extern void* _end;
/* Entry-point del modo protegido luego de cargar los registros de
 * segmento y armar un stack */
void kernel_init(void) {
    vga_init();
    idt_init();
    serial_init();
    debug_init();
    mm_init();
    swap_init();
    sched_init();
    con_init();

    hdd_init();
    ext2_init();
    fs_init();
    pipe_init();

    loader_init();
}
