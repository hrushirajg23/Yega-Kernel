#include "timer.h"
#include "registers.h"
#include "serial.h"

int timer_ticks = 0;

void timer_driver(registers_t *regs) {
  timer_ticks++;

  if (timer_ticks % 100 == 0) {
    serial_writestring("Timer tick: ");
    serial_writeint(timer_ticks);
    serial_writestring("\n");
  }
}
