/**
 * @file  allocator.c
 * @brief Custom physical memory allocator
 *        Functionalities to allocate and free memory blocks
 * @date 2025-07-14
 */

#include <stddef.h>
#include <stdint.h>

#include "allocator.h"
#include "serial.h"
#include "utils.h"

heap_block_t *head = NULL;

/* Allocate memory */
void *kalloc(size_t req) {
  req = ALIGNUP(req, ALIGN);
  size_t total_req = req + sizeof(heap_block_t);

  if ((kalloc_ptr + total_req) > heap_end) {
    serial_writestring("Not enough memory!\n");
    return NULL;
  }

  heap_block_t *curr = head;
  heap_block_t *prev = NULL;

  while (curr) {
    if (curr->is_freed && curr->size >= req) {
      curr->is_freed = false;
      return (void *)(curr + 1);
    }
    prev = curr;
    curr = curr->next;
  }

  curr = (heap_block_t *)kalloc_ptr;
  curr->size = req;
  curr->is_freed = false;
  curr->next = NULL;

  if (!head)
    head = curr;
  else
    prev->next = curr;

  kalloc_ptr += total_req;

  return (void *)(curr + 1);
}

/* Free memory 
 * kernel bump free
 */
void kbfree(void *ptr) {
  if (!ptr)
    return;

  heap_block_t *node = ((heap_block_t *)ptr) - 1;
  node->is_freed = true;

  if (!node->next)
    kalloc_ptr -= node->size + sizeof(heap_block_t);
}
