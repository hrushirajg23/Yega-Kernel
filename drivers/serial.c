/**
 * @file  serial.c
 * @brief Functionality to write on serial port
 *        from OSDev
 * @date 2025-07-14
 */

#include <stdint.h>
#include <stdarg.h>
#include "io_access.h"
#include "serial.h"

#define COM1 0x3F8

/* This function converts an integer into a character buffer */
static void get_digits(char *buf, int num, int *i) {
  if (num == 0) {
    buf[(*i)++] = '0';
    return;
  }

  int is_negative = 0;
  if (num < 0) {
    is_negative = 1;
    num = -num;
  }

  while (num > 0) {
    buf[(*i)++] = '0' + (num % 10);
    num /= 10;
  }

  if (is_negative) {
    buf[(*i)++] = '-';
  }
}

/* Initializes the serial port with specific configurations */
void serial_init() {
  outb(COM1 + 1, 0x00); // Disable all interrupts
  outb(COM1 + 3, 0x80); // Enable DLAB (set baud rate divisor)
  outb(COM1 + 0, 0x03); // Set divisor to 3 (io byte) 38400 baud
  outb(COM1 + 1, 0x00); // hi byte
  outb(COM1 + 3, 0x03); // 8 bits, no parity, one stop bit
  outb(COM1 + 2, 0xC7); // Enable FIFO, clear them, with 14-byte threshold
  outb(COM1 + 4, 0x0B); // IRQs enabled, RTS/DSR set
}

/* Writes a single character to the serial port */
void serial_writechar(char c) {
  while (!(inb(COM1 + 5) & 0x20))
    ;
  outb(COM1, c);
}

/* Writes a string to the serial port */
void serial_writestring(const char *str) {
  while (*str)
    serial_writechar(*str++);
}

/* Writes an integer to the serial port */
void serial_writeint(int num) {
  char buf[12];
  int i = 0;
  get_digits(buf, num, &i);

  while (i--) {
    serial_writechar(buf[i]);
  }
}

/* Writes a hex number to the serial port */
void serial_writehex(uint32_t num) {
  char hex_chars[] = "0123456789ABCDEF";

  serial_writestring("0x");

  for (int i = 28; i >= 0; i -= 4) {
    char c = hex_chars[(num >> i) & 0xF];
    serial_writechar(c);
  }
}

void serial_writedec(int num) {

    char buf[16];
    register int iCnt = 0;
    if(num == 0){
        serial_writestring("0");
        return;
    }

    if ( num < 0 ){
        serial_writestring("-");
        num = -num;
    }

    while ( num > 0 ){
        buf[iCnt++] = '0' + (num % 10);
        num /= 10;
    }
    while ( iCnt-- ){
        serial_writechar(buf[iCnt]);
    }

}

void printk(const char *format, ...)
{
    va_list args;
    va_start(args, format);

    char ch;
    while ((ch = *format++) != '\0'){
        if ( ch == '%' ){
            switch(*format++){
                case 'd':
                    serial_writedec(va_arg(args, int));
                    break;
                case 'u':
                    switch(*(format+1)){
                        case 'l':
                            serial_writedec(va_arg(args, unsigned long));
                            break;
                        default:
                            serial_writedec(va_arg(args, unsigned int));
                            break;
                    }
                case 'l':
                    serial_writedec(va_arg(args, long));
                    break;
                case 's':
                    serial_writestring(va_arg(args, char*));
                    break;
                case 'c':
                    serial_writechar((char)va_arg(args, int));
                    break;
                case 'x':
                    serial_writehex(va_arg(args, unsigned int));
                    break;
                case 'p':
                    serial_writedec(va_arg(args, unsigned long));
                    break;
                default:
                        serial_writechar('%');
                        serial_writechar(*(format - 1));
                        break;
            }
        }
    }
    va_end(args);
}
