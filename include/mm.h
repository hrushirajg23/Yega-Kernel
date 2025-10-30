#ifndef _MM_H
#define _MM_H

#include "zone.h"

#define ALIGN_PAGE(address) (address - (address % PAGE_SIZE))
#define IS_PAGE_ALIGNED(address) (((address) % PAGE_SIZE) == 0)

#define CLEAR_FLAG(var, flag) ((var &= ~(flag)))
#define SET_FLAG(var, flag) ((var |= (flag)))


void memset(void *addr, char val, unsigned int size);
void *memcpy(void *dest, void *src, unsigned int size);
void init_mem(multiboot_info_t *);

#endif
