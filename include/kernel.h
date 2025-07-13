#ifndef KERNEL_H
#define KERNEL_H

#include <stdint.h>

#define OFFSET1 0x20
#define OFFSET2 0x28
#define FREQUENCY 100

void kernel_main(uint32_t magic, uint32_t addr);

#endif