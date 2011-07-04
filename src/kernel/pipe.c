#include <pipe.h>
#include <mm.h>
#include <utils.h>
#include <loader.h>
#include <errors.h>

#define MAX_PIPE_DEVS 32
#define MAX_PIPE_BUFS (MAX_PIPE_DEVS/2)
#define PIPE_BUF_SIZE 4096

#define IS_READ_END(pipe) (!!((pipe)->read))
#define IS_WRITE_END(pipe) (!!((pipe)->write))

#define OTHER_END(pipe) (pipe)
#define OTHER_END_CLOSED(pipe) CLOSED(OTHER_END(pipe))
#define CLOSED(pipe) ((pipe)->refcount == 0)

#define GET_CBUF(pipe) (&(pipe)->buf->cbuf)


typedef struct str_pipe_buf pipedev_buf_t;
struct str_pipe_buf {
    circular_buf_t cbuf;

    pipedev_buf_t *next;
    pipedev_buf_t *prev;
};

/* pipe_chardev */
typedef struct str_pipe pipe_chardev;
struct str_pipe {
	uint_32 clase;
	uint_32 refcount;
	chardev_flush_t* flush;
	chardev_read_t* read;
	chardev_write_t* write;
	chardev_seek_t* seek;

    pipedev_buf_t *buf;
    int waiting_process;

    pipe_chardev *next;
    pipe_chardev *prev;
};


static pipe_chardev pipedevs[MAX_PIPE_DEVS];
static pipedev_buf_t pipedev_bufs[MAX_PIPE_BUFS];

static pipe_chardev *free_pipedevs = NULL;
static pipedev_buf_t *free_pipedev_bufs = NULL;


static void initialize_pipe_buf(pipedev_buf_t* pbuf);
static void initialize_pipe(pipe_chardev* pcdev);
static void initialize_reader_pipe(pipe_chardev* this);
static void initialize_writer_pipe(pipe_chardev* this);

sint_32 pipe_open(chardev* pipes[2]) {
    if (IS_EMPTY(free_pipedevs))
    	return -ENOMEM;

    pipe_chardev* reader = POP(&free_pipedevs);
    pipe_chardev* writer = POP(&free_pipedevs);

    initialize_reader_pipe(reader);
    initialize_writer_pipe(writer);
    
    pipedev_buf_t *pbuf = POP(&free_pipedev_bufs);
    initialize_pipe_buf(pbuf);

    reader->buf = writer->buf = pbuf;
    reader->refcount = writer->refcount = 1;

    pipes[0] = (chardev *) reader;
    pipes[1] = (chardev *) writer;

    return 0;
}

void pipe_init(void) {
    CREATE_FREE_OBJS_LIST(free_pipedevs, pipedevs, MAX_PIPE_DEVS);
    CREATE_FREE_OBJS_LIST(free_pipedev_bufs, pipedev_bufs, MAX_PIPE_BUFS);
}

sint_32 pipe_read(chardev* this, void* buf, uint_32 size) {
    if (this->clase != DEVICE_PIPE_CHARDEV)
        return -1;

    pipe_chardev *pcdev = (pipe_chardev *)this;

    while (GET_CBUF(pcdev)->remaining == 0 && !OTHER_END_CLOSED(pcdev))
        loader_enqueue(&pcdev->waiting_process);

    if (OTHER_END_CLOSED(pcdev))
        return 0;
    else {
        loader_unqueue(&OTHER_END(pcdev)->waiting_process);

        return read_from_circ_buff((char *)buf, GET_CBUF(pcdev), size,
            PIPE_BUF_SIZE);
    }
}

sint_32 pipe_write(chardev* this, const void* buf, uint_32 size) {
    if (this->clase != DEVICE_PIPE_CHARDEV)
        return -1;

    pipe_chardev *pcdev = (pipe_chardev *)this;

    while (GET_CBUF(pcdev)->remaining == PIPE_BUF_SIZE && !OTHER_END_CLOSED(pcdev))
        loader_enqueue(&pcdev->waiting_process);

    if (OTHER_END_CLOSED(pcdev))
        return 0;
    else {
        loader_unqueue(&OTHER_END(pcdev)->waiting_process);

        return write_to_circ_buff(GET_CBUF(pcdev), (char *)buf, size,
            PIPE_BUF_SIZE);
    }
}

uint_32 pipe_flush(chardev* this) {
    pipe_chardev *pcdev = (pipe_chardev *)this;

    if (CLOSED(pcdev))
        return -EFILEERROR;
    
    pcdev->refcount--;

    if (pcdev->refcount == 0) {
        if (OTHER_END_CLOSED(pcdev)) {
            mm_mem_free(GET_CBUF(pcdev)->buf);
        }
        else {
            while (OTHER_END(pcdev)->waiting_process != -1)
                loader_unqueue(&OTHER_END(pcdev)->waiting_process);
        }
    }

	return 0;
}

static void initialize_pipe_buf(pipedev_buf_t* pbuf) {
    pbuf->cbuf = ((circular_buf_t) { 
        .buf = mm_mem_kalloc(),
        .offset = 0,
        .remaining = 0
    });
}

static void initialize_pipe(pipe_chardev* pcdev) {
    pcdev->clase = DEVICE_PIPE_CHARDEV;
    pcdev->refcount = 0;
    pcdev->flush = pipe_flush;

    pcdev->waiting_process = -1;
}

static void initialize_reader_pipe(pipe_chardev* pcdev) {
    initialize_pipe(pcdev);
    pcdev->read = pipe_read;
    pcdev->write = NULL;
}

static void initialize_writer_pipe(pipe_chardev* pcdev) {
    initialize_pipe(pcdev);
    pcdev->write = pipe_write;
    pcdev->read = NULL;
}
