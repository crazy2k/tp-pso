export LC_ALL=C

# Programs
OBJDUMP=objdump
OBJCOPY=objcopy
CC=gcc
MKFS=/sbin/mkfs.vfat

# Parámetros de programas
NASMFLAGS=-I include/ -g -ggdb -D__KERNEL__
CFLAGS=-m32 -g -ggdb -Wall -O0 -I include/ \
  -fno-zero-initialized-in-bss -fno-stack-protector -fno-strict-aliasing -ffreestanding -D__KERNEL__
CXXFLAGS=-O0
LDFLAGS=-static -m elf_i386 -nostdlib

# Directorios
DIRBOOT=boot/
DIRKERN=kernel/
DIROUT=bin/
DIRTMP=/tmp/

# Boot loader
BIN_BOOT=$(DIRBOOT)floppyboot.bin

# Kernel
OBJS_KERN=kernel/kinit.o kernel/a20.o kernel/kernel.o kernel/gdt.o kernel/idt.o \
  kernel/isr.o kernel/pic.o kernel/vga.o kernel/sched.o \
  kernel/mm.o kernel/debug.o kernel/loader.o \
  kernel/syscalls.o kernel/sem.o kernel/utils.o kernel/loader_helpers.o \
  kernel/syscalls_funcs.o kernel/con.o kernel/device.o kernel/kb.o \
  kernel/serial.o kernel/hdd.o kernel/fs.o kernel/utils_common.o kernel/ext2.o \
  kernel/pipe.o
BIN_KERN=$(DIROUT)kernel.bin
DUMP_KERN=$(DIROUT)kernel.bin.asm $(DIROUT)kernel.bin.elf $(DIROUT)kernel.bin.dbg
SYM_KERN=$(DIROUT)kernel.sym

# Tareas
TASKS=tasks/task1.pso tasks/shell.pso
TASKS_ELF:=$(TASKS:.pso=.elf)
OBJS_TASKS:=$(TASKS:.pso=.o) tasks/pso_head.o tasks/pso_tail.o tasks/syscalls.o tasks/syscalls_funcs.o tasks/io.o kernel/utils_common.o

BIN_LIB:=tasks/syscalls_funcs.o tasks/io.o kernel/utils_common.o

# Archivos de texto
TEXTFILES=kernel/sem.c tasks/shell.c kernel/hdd.c kernel/ext2.c

# Imagenes
DISK_LABEL="KERNEL  PSO"
IMG_BASE=$(DIROUT)base.img
IMG_FLOPPY=$(DIROUT)floppy.img
IMG_TESTHDD=$(DIROUT)testhdd.img
IMG_TESTHDD_SIZE=10

.PHONY: all clean

all: $(IMG_FLOPPY) $(IMG_TESTHDD) $(DUMP_KERN) $(SYM_KERN)

# Bootloader - disk

# Kernel
kernel/%.o: kernel/%.asm
	nasm $(NASMFLAGS) -felf32 -I $(DIRKERN) -o $@ $<

tasks/pso_head.o: tasks/pso_head.asm
	nasm $(NASMFLAGS:__KERNEL__=__TASK__) -felf32 -I $(DIRKERN) -o $@ $<

tasks/pso_tail.o: tasks/pso_tail.asm
	nasm $(NASMFLAGS:__KERNEL__=__TASK__) -felf32 -I $(DIRKERN) -o $@ $<

tasks/syscalls.o: kernel/syscalls.asm
	nasm $(NASMFLAGS:__KERNEL__=__TASK__) -felf32 -I $(DIRKERN) -o $@ $<

tasks/%.pso: tasks/%.elf
	$(OBJCOPY) -S -O binary $< $@

tasks/%.o: tasks/%.c
	$(CC) $(CFLAGS:__KERNEL__=__TASK__) -c -o $@ $<

tasks/%.elf: tasks/pso_head.o tasks/syscalls.o tasks/%.o tasks/pso_tail.o $(BIN_LIB)
	$(LD) $(LDFLAGS) -N -e main -Ttext 0x400000 -o $@ tasks/pso_head.o tasks/syscalls.o $(BIN_LIB) $(@:.elf=.o) $(LDPSO) tasks/pso_tail.o

$(BIN_KERN).elf: $(OBJS_KERN) 
	$(LD) $(LDFLAGS) -N -e start -Ttext 0x1200 -o $@ $(OBJS_KERN) $(LDPSO)

$(BIN_KERN): $(BIN_KERN).elf
	$(OBJCOPY) -S -O binary $< $@

$(BIN_KERN).asm: $(BIN_KERN).elf
	$(OBJDUMP) -S -M intel $< >$@
	
$(BIN_KERN).dbg: $(BIN_KERN).elf
	$(OBJCOPY) --only-keep-debug $< $@

$(SYM_KERN): $(BIN_KERN).elf 
	nm $< | cut -d ' ' -f 1,3 > $@

# Images
$(IMG_BASE):
	@dd if=/dev/zero of=$@ bs=512 count=2880 2>/dev/null
	$(MKFS) -F 12 -f 2 -n $(DISK_LABEL) $@

$(IMG_FLOPPY): $(BIN_BOOT) $(BIN_KERN) $(IMG_BASE) $(TASKS)
	[ -f $(IMG_FLOPPY) ] || dd if=$(IMG_BASE) of=$@ bs=512 count=2880 2>/dev/null
	dd if=$< of=$@ bs=1 count=3 conv=notrunc 2>/dev/null
	dd if=$< of=$@ bs=1 count=450 seek=62 skip=62 conv=notrunc 2>/dev/null
	mcopy -obi $@ $(BIN_KERN) ::kernel.bin
	for T in $(TASKS); do mcopy -obi $@ $$T ::`basename $$T`; done;

$(IMG_TESTHDD): $(TASKS) $(TEXTFILES)
	rm -f $@
	bximage -q -hd -mode=flat -size=$(IMG_TESTHDD_SIZE) $@
	mke2fs -F -q $@
	for T in $(TASKS); do \
		debugfs -w -R "write $$T `basename $$T`" $@ ; \
	done
	for F in $(TEXTFILES); do \
		debugfs -w -R "write $$F `basename $$F`" $@ ; \
	done

# Otros
clean:
	rm -f $(BIN_KERN) $(IMG_FLOPPY) $(IMG_BASE) $(OBJS_KERN) $(DUMP_KERN) \
		$(SYM_KERN) $(OBJS_TASKS) $(TASKS) $(TASKS:.pso=.elf) $(BIN_LIB) \
		$(IMG_TESTHDD)
	#rm -f deps

# Dependencias
deps: Makefile
	makedepend -I include/ kernel/*.c tasks/*.c $(OBJS_LIB:.o=.c) -f- > deps
	for f in kernel/*.asm; do nasm $(NASMFLAGS) -felf32 -I $(DIRKERN) -M $$f; done >> deps 2>/dev/null
#	$(CC) $(CFLAGS) -MM kernel/*.c $(OBJS_LIB:.o=.c) > deps

-include deps

# DO NOT DELETE
