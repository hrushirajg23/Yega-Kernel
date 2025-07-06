#include <stdint.h>

#include "serial.h"
#include "io_access.h"

#define COM1 0x3F8

void serial_init() {
    outb(COM1 + 1, 0x00);    // Disable interrupts
    outb(COM1 + 3, 0x80);    // Enable DLAB (set baud rate divisor)
    outb(COM1 + 0, 0x03);    // Set divisor to 3 (38400 baud)
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x03);    // 8 bits, no parity, one stop bit
    outb(COM1 + 2, 0xC7);    // Enable FIFO, clear them
    outb(COM1 + 4, 0x0B);    // Enable IRQs, RTS/DSR set
}

void serial_writechar(char c) {
    while (!(inb(COM1 + 5) & 0x20));  // Wait until transmitter empty
    outb(COM1, c);
}

void serial_writestring(const char* str) {
    while (*str)
        serial_writechar(*str++);
}
