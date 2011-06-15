#include <pic.h>
#include <idt.h>
#include <vga.h>
#include <mm.h>
#include <sched.h>
#include <gdt.h>
#include <debug.h>
#include <loader.h>
#include <syscalls.h>
#include <i386.h>
#include <con.h>
#include <serial.h>
#include <hdd.h>

extern void* _end;
/* Entry-point del modo protegido luego de cargar los registros de
 * segmento y armar un stack */
void kernel_init(void) {
    vga_init();
    idt_init();
    hdd_init();
    serial_init();
    debug_init();
    mm_init();
    sched_init();
    con_init();
    loader_init();
}
