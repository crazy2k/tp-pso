#include <pipe.h>
#include <mm.h>
#include <utils.h>
#include <loader.h>
#include <errors.h>

#define MAX_PIPE_DEVS 32
#define PIPE_BUF_SIZE 4096

#define IS_READ_END(pipe) (!!((pipe)->read))
#define IS_WRITE_END(pipe) (!!((pipe)->write))

#define READER_END(writer_pipe) \
    (&(reader_pipe_devs[writer_pipe - writer_pipe_devs]))
#define WRITER_END(reader_pipe) \
    (&(writer_pipe_devs[reader_pipe - reader_pipe_devs]))
#define OTHER_END(pipe) \
    (IS_READ_END(pipe) ? WRITER_END(pipe) : READER_END(pipe))
#define CLOSED(pipe) ((pipe)->refcount == 0)

#define GET_CBUF(pipe) (&(pipe_bufs[pipe - \
    (IS_READ_END(pipe) ? reader_pipe_devs : writer_pipe_devs)]))
#define CBUF_FULL(pipe) (GET_CBUF(pipe)->remaining == PIPE_BUF_SIZE)
#define CBUF_EMPTY(pipe) (GET_CBUF(pipe)->remaining == 0)

/* pipe_chardev */
typedef struct str_pipe pipe_chardev;
struct str_pipe {
	uint_32 clase;
	uint_32 refcount;
	chardev_flush_t* flush;
	chardev_read_t* read;
	chardev_write_t* write;
	chardev_seek_t* seek;

    int waiting_process;

    pipe_chardev *next;
    pipe_chardev *prev;
};


static pipe_chardev reader_pipe_devs[MAX_PIPE_DEVS] = { {0} };
static pipe_chardev writer_pipe_devs[MAX_PIPE_DEVS] = { {0} };
static circular_buf_t pipe_bufs[MAX_PIPE_DEVS] = { {0} };

static pipe_chardev *free_reader_pipe_devs = NULL;
static pipe_chardev *free_writer_pipe_devs = NULL;


static void initialize_pipe(pipe_chardev* pcdev);
static void initialize_reader_pipe(pipe_chardev* this);
static void initialize_writer_pipe(pipe_chardev* this);


void pipe_init(void) {
    CREATE_FREE_OBJS_LIST(free_reader_pipe_devs, reader_pipe_devs, MAX_PIPE_DEVS);
    CREATE_FREE_OBJS_LIST(free_writer_pipe_devs, writer_pipe_devs, MAX_PIPE_DEVS);
}

sint_32 pipe_open(chardev* pipes[2]) {
    if (IS_EMPTY(free_reader_pipe_devs))
    	return -ENOMEM;

    pipe_chardev* reader = POP(&free_reader_pipe_devs);
    pipe_chardev* writer = WRITER_END(reader);

    UNLINK_NODE(&free_writer_pipe_devs, writer);

    initialize_reader_pipe(reader);
    initialize_writer_pipe(writer);
    
    *GET_CBUF(reader) = ((circular_buf_t) {
        .buf = mm_mem_kalloc(),
        .offset = 0,
        .remaining = 0
    });


    reader->refcount = writer->refcount = 1;

    pipes[0] = (chardev *) reader;
    pipes[1] = (chardev *) writer;

    return 0;
}

sint_32 pipe_read(chardev* this, void* buf, uint_32 size) {
    if (this->clase != DEVICE_PIPE_CHARDEV)
        return -1;

    pipe_chardev *pcdev = (pipe_chardev *)this;

    while (CBUF_EMPTY(pcdev) && !CLOSED(WRITER_END(pcdev)))
        loader_enqueue(&pcdev->waiting_process);

    if (!CLOSED(WRITER_END(pcdev)))
        loader_unqueue_all(&WRITER_END(pcdev)->waiting_process);

    return read_from_circ_buff((char *)buf, GET_CBUF(pcdev), size,
        PIPE_BUF_SIZE);
}

sint_32 pipe_write(chardev* this, const void* buf, uint_32 size) {
    if (this->clase != DEVICE_PIPE_CHARDEV)
        return -1;

    pipe_chardev *pcdev = (pipe_chardev *)this;

    while (CBUF_FULL(pcdev) && !CLOSED(READER_END(pcdev)))
        loader_enqueue(&pcdev->waiting_process);

    if (CLOSED(READER_END(pcdev)))
        return 0;
    else {
        loader_unqueue_all(&READER_END(pcdev)->waiting_process);

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
        if (CLOSED(OTHER_END(pcdev))) {
            mm_mem_free(GET_CBUF(pcdev)->buf);
            APPEND(&free_reader_pipe_devs, 
                IS_READ_END(pcdev) ? pcdev : READER_END(pcdev));
            APPEND(&free_writer_pipe_devs, 
                IS_WRITE_END(pcdev) ? pcdev : WRITER_END(pcdev));
        }
        else
            loader_unqueue_all(&OTHER_END(pcdev)->waiting_process);
    }

	return 0;
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
