#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "gdt.h"
#include "idt.h"
#include "irq.h"
#include "pic.h"
#include "pit.h"
#include "serial.h"
#include "vga_display.h"


#if defined(__linux__)
#error                                                                         \
    "You are not using a cross-compiler, you will most certainly run into trouble"
#endif

#if !defined(__i386__)
#error "This tutorial needs to be compiled with a ix86-elf compiler"
#endif

#define OFFSET1 0x20
#define OFFSET2 0x28
#define FREQUENCY 100


void kernel_main(void) {
  serial_init();
  serial_writestring("Serial initialized. Booting...\n");

  terminal_initialize();
  terminal_writestring("Hello, Welcome To Yega Kernel!\n");

  serial_writestring("GDT init...\n");
  gdt_initialize();

  serial_writestring("PIC remap...\n");
  remap_pic(OFFSET1, OFFSET2);

  serial_writestring("IDT setup...\n");
  setup_idt();

  serial_writestring("Install timer & keyboard drivers..\n");
  install_handlers();

  serial_writestring("PIT init...\n");
  init_timer(FREQUENCY);

  serial_writestring("IDT init...\n");
  initialize_idt();

  serial_writestring("Boot complete.\n");
}