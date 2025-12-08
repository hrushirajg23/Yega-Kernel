/**
 * @file  allocator.h
 * @brief Custom physical memory allocator
 * @date 2025-07-14
 */

#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define ALIGN 8

/* Heap block struct that contains size and pointer to next block*/
typedef struct heap_block {
  size_t size;
  bool is_freed;
  struct heap_block *next;
} heap_block_t;

void *kalloc(size_t req);
void kbfree(void *ptr);

#endif
