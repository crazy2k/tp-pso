#include <pic.h>
#include <i386.h>

#define PIC1_PORT 0x20
#define PIC2_PORT 0xA0

void pic_reset(uint_8 addr_pic1, uint_8 addr_pic2) {
	outb(PIC1_PORT+0, 0x11); /* IRQs activas x flanco, cascada, y ICW4 */
	outb(PIC1_PORT+1, addr_pic1); /* Addr */
	outb(PIC1_PORT+1, 0x04); /* PIC1 Master, Slave ingresa Int.x IRQ2 */
	outb(PIC1_PORT+1, 0x01); /* Modo 8086 */
	outb(PIC1_PORT+1, 0xFF); /* Enmasca todas! */

	outb(PIC2_PORT+0, 0x11); /* IRQs activas x flanco, cascada, y ICW4 */
	outb(PIC2_PORT+1, addr_pic2); /* Addr */
	outb(PIC2_PORT+1, 0x02); /* PIC2 Slave, ingresa Int x IRQ2 */
	outb(PIC2_PORT+1, 0x01); /* Modo 8086 */
	outb(PIC2_PORT+1, 0xFF); /* Enmasca todas! */

	return;
}

void pic_enable() {
	outb(PIC1_PORT+1, 0x00);
	outb(PIC2_PORT+1, 0x00);
}

void pic_disable() {
	outb(PIC1_PORT+1, 0xFF);
	outb(PIC2_PORT+1, 0xFF);
}

void remap_PIC(char offset1, char offset2) {
    // Inicializamos PIC1

    // Se envia ICW4, hay varios 8259A, interrupt vectors de 8 bytes,
    // interrupciones por flanco e inicializacion (ICW1)
    outb(PIC1_COMMAND, ICW1_INIT + ICW1_ICW4);
    outb(PIC1_DATA, offset1);       // Offset de interrupciones a la CPU (ICW2)
    outb(PIC1_DATA, ICW3_MATTACH2); // El slave ingresa por IRQ2 (ICW3)
    outb(PIC1_DATA, ICW4_8086);     // Modo 8086 (ICW4)

    // Enmascaramos las interrupciones del PIC1
    outb(PIC1_DATA, 0xFF);

    // Inicializamos PIC2

    outb(PIC2_COMMAND, ICW1_INIT + ICW1_ICW4);  // Idem PIC1 (ICW1)
    outb(PIC2_DATA, offset2);       // Offset de interrupciones a la CPU (ICW2)
    outb(PIC2_DATA, ICW3_SATTACH2); // Se ingresa al PIC1 por IRQ2 (ICW3)
    outb(PIC2_DATA, ICW4_8086);     // Modo 8086 (ICW4)
    
    // Enmascaramos las interrupciones del PIC2
    outb(PIC2_DATA, 0xFF);
}


void pic_set_mask(uint8_t line) {
    uint16_t port;

    if (line < 8)
        port = PIC1_DATA;
    else {
        port = PIC2_DATA;
        line -= 8;
    }
    outb(port, (inb(port) | (1 << line)));
}

void pic_clear_mask(uint8_t line) {
    uint16_t port;

    if (line < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        line -= 8;
    }
    outb(port, (inb(port) & ~(1 << line)));
}
