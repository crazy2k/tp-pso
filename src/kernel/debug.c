#include <debug.h>
#include <isr.h>
#include <idt.h>
#include <vga.h>
#include <mm.h>
#include <sched.h>
#include <i386.h>

const char* exp_name[] = {
	"Divide Error",
	"Debug Interrupt",
	"NMI Interrupt",
	"Breakpoint",
	"Interrupt on overflow",
	"BOUND range exceeded",
	"Invalid Opcode",
	"Device not available",
	"Double fault",
	"Coprocessor Segment Overrun",
	"Invalid TSS",
	"Segment not present",
	"Stack exception",
	"General protection fault",
	"Page fault",
	"Reserved",
	"Floating point exception",
	"Alignment check"
};





#define DEBUG_KP_REGS_COL 60
#define DEBUG_KP_REGS_INITROW 3
#define DEBUG_KP_COL 0
#define DEBUG_KP_ROW 0
#define DEBUG_KP_ATTR 0x0F
#define DEBUG_KP_STACK_ROW 2
#define DEBUG_KP_STACK_COL 2
#define DEBUG_KP_STACK_PPR 4        // Posiciones por fila
#define DEBUG_KP_STACK_ROWS 13
#define DEBUG_KP_BT_ROW DEBUG_KP_STACK_ROW + 1 + \
    DEBUG_KP_STACK_ROWS + 1
#define DEBUG_KP_BT_COL DEBUG_KP_STACK_COL
#define DEBUG_KP_BT_ROWS 4
bool in_panic = FALSE;
void debug_kernelpanic(const uint_32* stack, const exp_state* expst) {
	/* No permite panics anidados */
	if (in_panic) while(1) hlt();
	in_panic = TRUE;

    vga_cls();
    vga_printf(DEBUG_KP_ROW, DEBUG_KP_COL, "Kernel Panic!",
        DEBUG_KP_ATTR);

    // Imprimimos los registros
    char *regs[] = { "eax: %x", "ebx: %x", "ecx: %x", "edx: %x", "esi: %x",
        "edi: %x", "ebp: %x", "esp: %x", "eip: %x", "efl: %x", "tr:  %x",
        "cr2: %x", "cr3: %x" };
    uint32_t regs_values[] = { expst->eax, expst->ebx, expst->ecx,
        expst->edx, expst->esi, expst->edi, expst->ebp, expst->esp,
        expst->org_eip, expst->eflags, rtr(), rcr2(), rcr3() };

    uint16_t regs_row = DEBUG_KP_REGS_INITROW;
    int i;
    for (i = 0; i < (sizeof(regs)/sizeof(char *)); i++)
        vga_printf(regs_row++, DEBUG_KP_REGS_COL, regs[i], DEBUG_KP_ATTR,
            regs_values[i]);


    // Imprimimos el stack
     vga_printf(DEBUG_KP_STACK_ROW, DEBUG_KP_STACK_COL, "Stack:",
         DEBUG_KP_ATTR);

    const uint32_t *addr = stack;
    uint16_t stack_row = DEBUG_KP_STACK_ROW + 1;
    for (addr = stack; addr < stack + DEBUG_KP_STACK_PPR*DEBUG_KP_STACK_ROWS;
        addr += DEBUG_KP_STACK_PPR) {
        vga_printf(stack_row++, DEBUG_KP_STACK_COL, "%x: %x %x %x %x",
            DEBUG_KP_ATTR, (uint32_t)addr, *(addr), *(addr + 1),
            *(addr + 2), *(addr + 3));
    }

    // Imprimimos el backtrace
    uint16_t bt_row = DEBUG_KP_BT_ROW;
    vga_printf(bt_row++, DEBUG_KP_BT_COL, "Backtrace: Current: %x",
        DEBUG_KP_ATTR, expst->org_eip);

    uint32_t *ebp = (uint32_t *)expst->ebp;
    uint32_t ret_addr = *(ebp + 1);
    for (i = 0; i < DEBUG_KP_BT_ROWS; i++) {
        uint32_t *first_parm = ebp + 2;
        // XXX: Ver si asumir lo siguiente no es demasiado fuerte
        // 5 bytes por el opcode del call relativo
        uint32_t at = ret_addr - 5;

        vga_printf(bt_row++, DEBUG_KP_BT_COL, "At %x: CALL No se "
            "(%x, %x, %x, %x, ...)", DEBUG_KP_ATTR, at, *first_parm,
            *(first_parm + 1), *(first_parm + 2), *(first_parm + 3));

        ebp = (uint32_t *)(*ebp);
        ret_addr = *(ebp + 1);
    }

}


void debug_init(void) {
    int i;
    for (i = 0; i < IDT_LENGTH; i++)
        idt_set_isr(i, gather_and_panic_noerrcode, IDT_DESC_DPL(0) |
            IDT_DESC_P | IDT_DESC_D | IDT_DESC_INT);
}

