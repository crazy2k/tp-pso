#ifndef __PIC_H__
#define __PIC_H__

// Constantes de los PIC y sus ICWs

#define PIC1 0x20
#define PIC2 0xA0
#define PIC1_COMMAND PIC1
#define PIC1_DATA (PIC1 + 1)
#define PIC2_COMMAND PIC2
#define PIC2_DATA (PIC2 + 1)

// Initialization Command Words para los PICs
#define ICW1_ICW4       0x01    // ICW4 se va a enviar (o no)
#define ICW1_SINGLE     0x02    // Un unico 8259A (o mas de uno)
#define ICW1_INTERVAL4  0x04    // Interrupt vectors de 4 bytes (o de 8)
#define ICW1_LEVEL      0x08    // Interrupciones por nivel (o por flanco)
#define ICW1_INIT       0x10    // Inicializacion

#define ICW3_MATTACH2   0x04    // El slave ingresa por IRQ2
#define ICW3_SATTACH2   0x02    // Se ingresa al master por IRQ2

#define ICW4_8086       0x01    // Modo 8086 (u 8085)
#define ICW4_AUTO       0x02    // EOI automatico (o normal)
#define ICW4_MASTER     0x04    // Master (o esclavo)
#define ICW4_BUFFERED   0x08    // Buffered mode (o nonbuffered)
#define ICW4_SFNM       0x10    // Fully nested (o not fully nested)

#define OCW2            0x20    // EOI (end-of-interrupt)

#define PIC1_OFFSET 0x20
#define PIC2_OFFSET 0x28

#define PIC_TIMER       0x1
#define PIC_KB          0x2

#define PIC_ALL_ENABLED ((~PIC_TIMER) & (~PIC_KB))


void remap_PIC(char offset1, char offset2);
void pic_reset(uint_8 addr_pic1, uint_8 addr_pic2);
void pic_enable();
void pic_disable();


#endif
