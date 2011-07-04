#include <pipe.h>
#include <errors.h>


/* pipedev */
typedef struct str_pipe {
	/* ... completar ...*/
// DELETE START
// DELETE END
} pipedev;

sint_32 pipe_open(chardev* pipes[2]) {
	return -ENOSYS;
}

void pipe_init(void) {

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
