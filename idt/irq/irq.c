#include <stdint.h>

#include "irq.h"
#include "pic.h"
#include "serial.h"
#include "keyboard.h"
#include "timer.h"

typedef void (*irq_handler_t)(registers_t *);

irq_handler_t irq_handlers[16];

void install_handlers(void) {
  irq_handlers[0] = timer_driver;
  irq_handlers[1] = keyboard_driver;
}

void irq_handler(registers_t *regs) {
  uint8_t int_num = (uint8_t)regs->int_no - 32;

  serial_writestring("Recieved IRQ: ");
  serial_writeint(int_num);

  if (irq_handlers[int_num])
    irq_handlers[int_num](regs);

  send_EOI(int_num);
}


