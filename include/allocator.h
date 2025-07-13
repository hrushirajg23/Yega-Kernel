#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define ALIGN 8

typedef struct heap_block {
  size_t size;
  bool is_freed;
  struct heap_block *next;
} heap_block_t;

void *kalloc(size_t req);
void kfree(void *ptr);

#endif