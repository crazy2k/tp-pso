#include <syscalls.h>
#include <mm.h>
#include <pipe.h>
#include <fs.h>
#include <loader.h>
#include <sched.h>
#include <errors.h>
#include <vga.h>
#include <debug.h>

void sys_exit() {
    loader_exit();
}

uint32_t sys_getpid() {
    return sched_get_current_pid();
}

void *sys_palloc() {
    return mm_request_mem_alloc();
}

int sys_read(int fd, void *buf, uint32_t size) {
    chardev *cdev;
    if (!(cdev = loader_get_file(fd)))
        return -ENOFILE;

    if (!(cdev->read))
        return -ENOFUNC;

    return cdev->read(cdev, buf, size);
}

int sys_write(int fd, const void *buf, uint32_t size) {
    chardev *cdev;
    if (!(cdev = loader_get_file(fd)))
        return -ENOFILE;

    if (!(cdev->write))
        return -ENOFUNC;

    return cdev->write(cdev, buf, size);
}

int sys_seek(int fd, uint32_t size) {
    chardev *cdev;
    if (!(cdev = loader_get_file(fd)))
        return -ENOFILE;

    if (!(cdev->seek))
        return -ENOFUNC;

    return cdev->seek(cdev, size);
}

int sys_close(int fd) {
    chardev *cdev;
    if (!(cdev = loader_get_file(fd)))
        return -ENOFILE;

    // Si hay mas referencias, entonces aun no hay que hacer flush
    if ((--(cdev->refcount)) > 0)
        return 0;

    if (!(cdev->flush))
        return -ENOFUNC;

    return cdev->flush(cdev);
}

int sys_open(char *path, uint32_t mode) {
    chardev *cdev; 
    if (!(cdev = fs_open(path, mode)))
        return -ENOFILE;
    else {
        cdev->refcount++;
        return loader_add_file(cdev);
    }
}


int sys_pipe(int fds[2]) {
    chardev* pipes[2];
    int result = pipe_open(pipes);

    fds[0] = loader_add_file(pipes[0]);
    fds[1] = loader_add_file(pipes[1]);

    return result; 
}

int sys_fork(task_state_t* st) {
    return loader_fork(st);
}

int sys_share_page(void *page) {
    return mm_share_page(page);
}

int sys_run(const char *path) {
    chardev *cdev;
    if (!(cdev = fs_open(path, FS_OPEN_RDONLY)))
        return -1;

    pso_file *pso_file = mm_mem_kalloc();
    cdev->read(cdev, pso_file, PAGE_SIZE);

    return loader_load(pso_file, 3);
}


