/**
 * @file  utils.h
 * @brief Utility functions for memory management
 * @date 2025-07-14
 */

#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>
#include <stdint.h>
#include "manager.h"

#define ALIGNUP(offset, align) ((offset) + ((align) + 1) & ~((align)-1))

extern uintptr_t heap_start, heap_end, kalloc_ptr;
extern size_t heap_size;

typedef struct memory_blocks {
  uint64_t start;
  uint64_t end;
} __attribute__((packed)) memory_blocks_t;

extern memory_blocks_t free_memory_blocks[MAX_MEMORY_BLOCKS];

#endif
