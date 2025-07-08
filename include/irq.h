#ifndef IRQ_H
#define IRQ_H

#include "registers.h"

void irq_handler(registers_t *regs);
void install_handlers(void);

#endif