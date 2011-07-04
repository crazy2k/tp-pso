#include <pipe.h>
#include <utils.h>
#include <errors.h>

#define MAX_PIPE_DEVS 32

/* pipedev */
typedef struct str_pipe pipedev;
struct str_pipe {
	uint_32 clase;
	uint_32 refcount;
	chardev_flush_t* flush;
	chardev_read_t* read;
	chardev_write_t* write;
	chardev_seek_t* seek;

    circular_buf_t buf;
    int waiting_process;

    pipedev *next;
    pipedev *prev;
};


static pipedev pipedevs[MAX_PIPE_DEVS] = { {0} };
static pipedev *free_pipedevs = NULL;

sint_32 pipe_open(chardev* pipes[2]) {
	return -ENOSYS;
}

void pipe_init(void) {
    CREATE_FREE_OBJS_LIST(free_pipedevs, pipedevs, MAX_PIPE_DEVS);
}

sint_32 pipe_read(chardev* this, void* buf, uint_32 size) {
	return 0;
}

sint_32 pipe_write(chardev* this, const void* buf, uint_32 size) {
	return 0;
}

uint_32 pipe_flush(chardev* this) {
	return 0;
}
