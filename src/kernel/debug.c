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





#define REGS_COL 50
bool in_panic = FALSE;
void debug_kernelpanic(const uint_32* stack, const exp_state* expst) {
	/* No permite panics anidados */
	if (in_panic) while(1) hlt();
	in_panic = TRUE;

    char *regs[] = { "eax: %x", "ebx: %x", "ecx: %x", "edx: %x", "esi: %x",
        "edi: %x", "ebp: %x", "esp: %x", "eip: %x", "efl: %x" };

    uint32_t regs_values[] = { expst->eax, expst->ebx, expst->ecx,
        expst->edx, expst->esi, expst->edi, expst->ebp, expst->esp,
        expst->org_eip, expst->eflags };

    uint16_t regs_row = 3;
    int i;
    for (i = 0; i < (sizeof(regs)/sizeof(char)); i++)
        vga_printf(regs_row++, REGS_COL, regs[i], 0x0F, regs_values[i]);

}


void debug_init(void) {
    int i;
    for (i = 0; i < IDT_LENGTH; i++)
        idt_set_isr(i, gather_and_panic_noerrcode, IDT_DESC_DPL(0) |
            IDT_DESC_P | IDT_DESC_D | IDT_DESC_INT);
}

