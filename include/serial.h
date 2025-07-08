#ifndef SERIAL_H
#define SERIAL_H

void serial_init();
void serial_writechar(char c);
void serial_writestring(const char* str);
void serial_writeint(int num);
void serial_writehex(uint32_t num);

#endif
