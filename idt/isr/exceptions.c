#include <stdint.h>

#include "exceptions.h"
#include "serial.h"


void exception_handler(registers_t *regs) {
  const char *message = exception_messages[regs->int_no];
  int int_num = regs->int_no;
  uint32_t err = regs->err_code;
  // uint32_t eip = regs->eip;
  // uint32_t cs = regs->cs;
  // uint32_t eflags = regs->eflags;
  uint32_t eax = regs->eax;
  uint32_t esi = regs->esi;
  uint32_t ds = regs->ds;
  uint32_t ebx = regs->ebx;
  uint32_t ecx = regs->ecx;
  uint32_t edx = regs->edx;
  uint32_t edi = regs->edi;
  uint32_t ebp = regs->ebp;

  serial_writestring("Exception: ");
  serial_writestring(message);

  serial_writestring("\nInterrupt Number: ");
  serial_writeint(int_num);

  serial_writestring("\nEAX: ");
  serial_writehex(eax);

  serial_writestring("\nEBX: ");
  serial_writehex(ebx);

  serial_writestring("\nECX: ");
  serial_writehex(ecx);

  serial_writestring("\nEDX: ");
  serial_writehex(edx);

  serial_writestring("\nESI: ");
  serial_writehex(esi);

  serial_writestring("\nEDI: ");
  serial_writehex(edi);

  serial_writestring("\nEBP: ");
  serial_writehex(ebp);

  serial_writestring("\nDS: ");
  serial_writehex(ds);

  while (1)
    __asm__ volatile("cli; hlt");
}