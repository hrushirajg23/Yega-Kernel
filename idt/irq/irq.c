/**
 * @file  irq.c
 * @brief Interrupt Request Handlers
 *        and installing drivers
 * @date 2025-07-14
 */

#include <stdint.h>

#include "irq.h"
#include "pic.h"
#include "serial.h"
#include "keyboard.h"
#include "timer.h"

typedef void (*irq_handler_t)(registers_t *);

irq_handler_t irq_handlers[16];

/* Install drivers */
void install_handlers(void) {
  irq_handlers[0] = timer_driver;
  irq_handlers[1] = keyboard_driver;
}

/* Interrupt handler */
void irq_handler(registers_t *regs) {
  serial_writestring("RAW int_no: ");
  serial_writeint(regs->int_no);
  serial_writestring("\n");

  uint8_t irq = regs->int_no - 32;
  serial_writestring("IRQ#: ");
  serial_writeint(irq);
  serial_writestring(" hit\n");

  if (irq_handlers[irq])
    irq_handlers[irq](regs);

  send_EOI(irq);
}

