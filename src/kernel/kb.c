#include <con.h>
#include <kb.h>
#include <vga.h>

#define KB_STATUS_SHIFT 0x0001
#define KB_STATUS_ALT 0x0002
#define KB_STATUS_CTRL 0x0004

static uint32_t kb_status = NULL;

static char sc2kc(uint8_t sc);


char sc2kc_table[KB_RELEASE_BASE] = {
    [0x00] = KB_KC_ERROR,
    [0x01] = 27,
    [0x02] = '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 8,
    [0x0F] = 9, 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']',
    [0x1C] = 13,
    [0x1D] = KB_KC_LCTRL,
    [0x1E] = 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', 39,
    [0x29] = '`',
    [0x2A] = KB_KC_LSHIFT,
    [0x2B] = '\\',
    [0x2C] = 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', KB_KC_RSHIFT,
    [0x37] = KB_KC_KEYPAD,
    [0x38] = KB_KC_LALT,
    [0x39] = ' ',
    [0x3A] = KB_KC_CAPSLOCK,
    [0x3B] = KB_KC_F(1), KB_KC_F(2), KB_KC_F(3), KB_KC_F(4), KB_KC_F(5),
    KB_KC_F(6), KB_KC_F(7), KB_KC_F(8), KB_KC_F(9), KB_KC_F(10),
    [0x45] = KB_KC_NUMLOCK,
    [0x46] = KB_KC_SCROLLLOCK,
    [0x47] = KB_KC_HOME, KB_KC_UP, KB_KC_PGUP,
    [0x4A] = KB_KC_MINUS,
    [0x4B] = KB_KC_LEFT, '5', KB_KC_RIGHT, '+',
    [0x4F] = KB_KC_END, KB_KC_DOWN, KB_KC_PGDN,
    [0x52] = KB_KC_INS, KB_KC_DEL,
    [0x54] = KB_KC_SYSRQ,
    [0x55] = KB_KC_F(11),
    [0x56] = KB_KC_UNLABELLED,
    [0x57] = KB_KC_F(11), KB_KC_F(12),
    [0x59 ... (KB_RELEASE_BASE - 1)] = KB_KC_NULL,
};

static char sc2kc(uint8_t sc) {
    int idx = (int)sc;
    return sc2kc_table[idx];
}

void kb_process_byte(uint8_t b) {
    uint32_t mask = 0; 

    if (!con_get_current_console())
        return;

    uint8_t kc = sc2kc(MASK_RELEASE(b));
    switch (kc) {
        case KB_KC_LSHIFT:
        case KB_KC_RSHIFT:
            mask = KB_STATUS_SHIFT;
            break;
        case KB_KC_LALT:
            mask = KB_STATUS_ALT;
            break;
        case KB_KC_LCTRL:
            mask = KB_STATUS_CTRL;
            break;
    }

    if (mask) {
        if (IS_KEY_RELEASE(b))
            kb_status |= mask;
        else 
            kb_status &= ~mask;
    } else if(!IS_KEY_RELEASE(b)) {
        if (kb_status & (KB_STATUS_SHIFT | KB_STATUS_ALT)) {
            switch (kc) { 
                case KB_KC_LEFT:
                    con_put_to_kb_buf(con_get_current_console(),
                        KB_KC_SHIFT_ALT_LEFT);
                    break;
                case KB_KC_RIGHT:
                    con_put_to_kb_buf(con_get_current_console(),
                        KB_KC_SHIFT_ALT_RIGHT);
                    break;
            }
        } else if (kc != KB_KC_ERROR && kc != KB_KC_NULL)
            con_put_to_kb_buf(con_get_current_console(), kc);
    }
}
