#ifndef PIC_H
#define PIC_H

#define PIC_EOI 0x20

#define PIC1 0x20 /* base address for master PIC */
#define PIC2 0xA0 /* base address for slave PIC */
#define PIC1_COMMAND PIC1
#define PIC1_DATA (PIC1 + 1)
#define PIC2_COMMAND PIC2
#define PIC2_DATA (PIC2 + 1)

void remap_pic(int offset1, int offset2);
void IRQ_set_mask(uint8_t IRQline);
void IRQ_clear_mask(uint8_t IRQline);
void send_EOI(uint8_t irq);

uint16_t pic_get_irr(void);
uint16_t pic_get_isr(void);

#endif