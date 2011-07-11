#include <con.h>
#include <device.h>
#include <vga.h>
#include <kb.h>
#include <utils.h>
#include <mm.h>
#include <debug.h>
#include <loader.h>

#define MAX_CON_CHARDEVS 16
#define KB_BUF_SIZE 1024
#define TAB_STR "    "
#define TAB_SIZE (sizeof(TAB_STR) - 1)
#define IS_FOCUSED(ccdev) ((ccdev) == current_console)


// XXX: Deberian ser static?
sint_32 con_read(chardev *this, void *buf, uint_32 size);
sint_32 con_write(chardev *this, const void *buf, uint_32 size);
uint_32 con_flush(chardev *this);


static void initialize_console(con_chardev *ccdev);
static void con_backspace(con_chardev *ccdev);
static void con_delete_cur_char(con_chardev *ccdev);
static void con_clear_screen(con_chardev *ccdev);
static void update_console_cursor(con_chardev *ccdev);
static void scroll_down(con_chardev *ccdev);

static con_chardev con_chardevs[MAX_CON_CHARDEVS] = { { NULL } };
static con_chardev *opened_con_chardevs = NULL;
static con_chardev *free_con_chardevs = NULL;

static con_chardev *current_console = NULL;



void con_init() {
    CREATE_FREE_OBJS_LIST(free_con_chardevs, con_chardevs, MAX_CON_CHARDEVS);
}


chardev* con_open(uint32_t number, uint32_t mode) {
    con_chardev* con;

    if (number < MAX_CON_CHARDEVS) {
        con = &con_chardevs[number];
        if (!IS_AT_LIST(opened_con_chardevs, con)) 
            initialize_console(con);

    } else if (number == NEW_CONSOLE && !IS_EMPTY(free_con_chardevs)) {
        con = POP(&free_con_chardevs);
        initialize_console(con);

    } else
        return NULL;

    con->refcount++;
    return (chardev*) con;
}

sint_32 con_read(chardev *this, void *buf, uint_32 size) {
     if (this->clase != DEVICE_CON_CHARDEV)
        return -1;

    con_chardev *ccdev = (con_chardev *)this;

    while (ccdev->kb_buf.remaining == 0)
        loader_enqueue(&ccdev->waiting_process);

    return read_from_circ_buff((char *)buf, &ccdev->kb_buf, size,
        KB_BUF_SIZE);
}

sint_32 con_write(chardev *this, const void *buf, uint_32 size) {
    if (this->clase != DEVICE_CON_CHARDEV)
        return -1;

    con_chardev *ccdev = (con_chardev *)this;
    char *cbuf = (char *)buf;

    void *out_base = IS_FOCUSED(ccdev) ? (void *)VGA_ADDR : ccdev->screen_buf;

    int i;
    for (i = 0; i < size; i++) {
        if (ccdev->screen_buf_offset >= VGA_SCREEN_SIZE)
            scroll_down(ccdev);

        int char_offset;

        if (cbuf[i] == '\n' || cbuf[i] == '\r')
            char_offset = VGA_ROW_SIZE - (ccdev->screen_buf_offset % VGA_ROW_SIZE);
        else if (cbuf[i] == '\t') {
            vga_puts(out_base + ccdev->screen_buf_offset, TAB_STR,
                ccdev->current_attr);
            char_offset = VGA_CHAR_SIZE * TAB_SIZE;
        }
        else { 
            vga_putchar(out_base + ccdev->screen_buf_offset, cbuf[i],
                ccdev->current_attr);
            char_offset = VGA_CHAR_SIZE;
        }
        ccdev->screen_buf_offset += char_offset;
    }

//    update_console_cursor((con_chardev *)this);
    return i;
}

uint_32 con_flush(chardev *this) {
    if (this->clase != DEVICE_CON_CHARDEV)
        return -1;

    con_chardev *ccdev = (con_chardev *)this;

    if (IS_FOCUSED(ccdev))
        con_focus(ccdev->next != ccdev ? ccdev->next : NULL);

    mm_mem_free(ccdev->screen_buf);
    mm_mem_free(ccdev->kb_buf.buf);

    UNLINK_NODE(&opened_con_chardevs, ccdev);
    APPEND(&free_con_chardevs, ccdev);

    return 0;
}

void con_focus(con_chardev *con) {
    if (current_console == con)
        return;

    if (current_console != NULL)
        memcpy(current_console->screen_buf, (void *)VGA_ADDR,
            VGA_SCREEN_SIZE);

    if (con)
        memcpy((void *)VGA_ADDR, con->screen_buf, VGA_SCREEN_SIZE);
    current_console = con;
//    update_console_cursor(con);
}

void con_put_to_kb_buf(con_chardev * ccdev, uint8_t b) {
    switch(b) {
        case KB_KC_SHIFT_ALT_LEFT:
            con_focus(ccdev->prev);
        break;
        case KB_KC_SHIFT_ALT_RIGHT:
            con_focus(ccdev->next);
        break;
        default:
            put_char_to_circ_buff(&ccdev->kb_buf, b, KB_BUF_SIZE);
            loader_unqueue_all(&ccdev->waiting_process);
    }
}

con_chardev *con_get_current_console() {
    return current_console;
}


void con_ctl(con_chardev *ccdev, uint32_t oper) {
    switch (oper) {
        case CON_CTL_DELETE_CUR_CHAR:
            con_delete_cur_char(ccdev);
        break;
        case CON_CTL_CLS_SCREEN:
            con_clear_screen(ccdev);
        break;
        case CON_CTL_BACKSPACE:
            con_backspace(ccdev);
    }
    
}

static void initialize_console(con_chardev *ccdev) {
    ccdev->clase = DEVICE_CON_CHARDEV;
    ccdev->refcount = 0;
    ccdev->flush = con_flush;
    ccdev->read = con_read;
    ccdev->write = con_write;

    ccdev->screen_buf = mm_mem_kalloc();
    ccdev->screen_buf_offset = 0;

    ccdev->kb_buf = ((circular_buf_t) { 
        .buf = mm_mem_kalloc(),
        .offset = 0,
        .remaining = 0
    });

    ccdev->waiting_process = -1 ;

    ccdev->current_attr = 0x0F;

    APPEND(&opened_con_chardevs, ccdev);

    if (!con_get_current_console())
        con_focus(ccdev);
}

static void con_backspace(con_chardev *ccdev) {
    if (ccdev->screen_buf_offset > 0) {
        ccdev->screen_buf_offset -= VGA_CHAR_SIZE;
        con_delete_cur_char(ccdev);
    }
}


static void con_delete_cur_char(con_chardev *ccdev) {
    void *addr = (IS_FOCUSED(ccdev) ? (void *)VGA_ADDR : ccdev->screen_buf) + 
        ccdev->screen_buf_offset;

    vga_putchar(addr, ' ', 0x0F);
}

static void con_clear_screen(con_chardev *ccdev) {
    void *addr = IS_FOCUSED(ccdev) ? (void *)VGA_ADDR : ccdev->screen_buf;
    void *end = addr + VGA_ROWS*VGA_COLS*VGA_CHAR_SIZE;

    for (; addr < end; addr += VGA_CHAR_SIZE)
        vga_putchar(addr, ' ', 0x0F);
    ccdev->screen_buf_offset = 0;
}

static void scroll_down(con_chardev *ccdev) {
    void *screen = IS_FOCUSED(ccdev) ? (void *)VGA_ADDR : ccdev->screen_buf;
    void *screen_limit = screen + VGA_SCREEN_SIZE;
    memcpy(screen, screen + VGA_ROW_SIZE,
        (screen_limit - screen) - VGA_ROW_SIZE);

    ccdev->screen_buf_offset -= VGA_ROW_SIZE;
    vga_clline(VGA_LINE_BEGIN(screen + ccdev->screen_buf_offset, screen));
}

static void update_console_cursor(con_chardev *ccdev) {
    vga_update_cursor(ccdev->screen_buf_offset / VGA_ROW_SIZE, ccdev->screen_buf_offset % VGA_ROW_SIZE);
}
