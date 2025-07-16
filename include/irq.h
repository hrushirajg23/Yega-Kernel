/**
 * @file  irq.h
 * @brief Functinalities to handle interupt requests (IRQs)
 * @date 2025-07-14
 */

#ifndef IRQ_H
#define IRQ_H

#include "registers.h"

void irq_handler(registers_t *regs);
void install_handlers(void);

#endif