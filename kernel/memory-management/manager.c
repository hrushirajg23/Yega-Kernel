/**
 * @file  gdt.c
 * @brief Setting up memory management
 *        Functionalities to find available memory blocks, setup heap, and test
 *        allocator
 * @date 2025-07-14
 */

#include <stdint.h>

#include "allocator.h"
#include "manager.h"
#include "serial.h"
#include "utils.h"

uintptr_t heap_start, heap_end, kalloc_ptr;
size_t heap_size = HEAP_SIZE;
memory_blocks_t free_memory_blocks[MAX_MEMORY_BLOCKS];

static int test_allocator();

/* Initialize memory manager */
int initialize_memeory_manager(multiboot_info_t *mbi) {
  int num_blocks;

  serial_writestring("\nflags= ");
  serial_writehex(mbi->flags);

  num_blocks = find_available_memory(mbi);
  if (num_blocks == 0) {
    serial_writestring("\nNo memory available.\n");
    return -1;
  } else {
    serial_writestring("\nMemory available.\n");
    print_blocks(num_blocks);
  }

  /* Initialize Heap */
  heap_start = free_memory_blocks[0].start;
  heap_start = ALIGNUP(heap_start, PAGE_SIZE);
  heap_end = heap_start + heap_size;
  kalloc_ptr = heap_start;

  return test_allocator();
}

/* Test allocator */
static int test_allocator() {
  serial_writestring("Starting allocator test...\n");

  // Allocate first array
  int *arr1 = (int *)kalloc(10 * sizeof(int));
  if (!arr1) {
    serial_writestring("arr1 allocation failed!\n");
    return -1;
  }

  for (int i = 0; i < 10; i++)
    arr1[i] = i * 2;

  // Verify
  for (int i = 0; i < 10; i++) {
    if (arr1[i] != i * 2) {
      serial_writestring("Memory corruption detected in arr1!\n");
      return -1;
    }
  }

  serial_writestring("arr1 write/read test passed.\n");

  // Allocate second array
  int *arr2 = (int *)kalloc(5 * sizeof(int));
  if (!arr2) {
    serial_writestring("arr2 allocation failed!\n");
    return -1;
  }

  for (int i = 0; i < 5; i++)
    arr2[i] = i + 100;

  // free the first array
  kfree(arr1);

  // Allocate third array
  int *arr3 = (int *)kalloc(8 * sizeof(int));
  if (!arr3) {
    serial_writestring("arr3 allocation failed!\n");
    return -1;
  }

  for (int i = 0; i < 8; i++)
    arr3[i] = i + 200;

  // Verify first array
  for (int i = 0; i < 5; i++) {
    if (arr2[i] != i + 100) {
      serial_writestring("Memory corruption detected in arr2!\n");
      return -1;
    }
  }

  serial_writestring("arr2 preserved correctly after arr3 allocation.\n");
  serial_writestring("Allocator test completed successfully.\n");

  return 0;
}

int find_available_memory(multiboot_info_t *mbi) {
  serial_writestring("\nmmap addr= ");
  serial_writehex(mbi->mmap_addr);

  serial_writestring("\nmmap length= ");
  serial_writehex(mbi->mmap_length);

  int num_block = 0;
  uint8_t *mmap = (uint8_t *)mbi->mmap_addr;
  uint8_t *mmap_end = mmap + mbi->mmap_length;

  serial_writestring("\nflags= ");
  serial_writehex(mbi->flags);

  if (!CHECK_FLAG(mbi->flags, 6))
    return 0;

  while (mmap < mmap_end) {
    multiboot_mmap_entry_t *entry = (multiboot_mmap_entry_t *)mmap;

    serial_writestring("\nentry addr= ");
    serial_writehex(entry->addr);

    serial_writestring("\nentry len= ");
    serial_writehex(entry->len);

    serial_writestring("\nentry type= ");
    serial_writehex(entry->type);

    serial_writestring("\nentry size= ");
    serial_writehex(entry->size);

    if (num_block < MAX_MEMORY_BLOCKS && entry->type == 1) {
      uint64_t entry_start = entry->addr;
      uint64_t entry_end = entry_start + entry->len;

      if (entry_start <= KERNEL_END && entry_end > KERNEL_END) {
        free_memory_blocks[num_block].start = KERNEL_END;
        free_memory_blocks[num_block].end = entry_end;
        num_block++;
      } else if (entry_start > KERNEL_END) {
        free_memory_blocks[num_block].start = entry_start;
        free_memory_blocks[num_block].end = entry_end;
        num_block++;
      }
    }

    mmap += entry->size + sizeof(entry->size);
  }

  return num_block;
}

void print_blocks(int num_blocks) {
  for (int i = 0; i < num_blocks; i++) {
    serial_writestring("\nblock ");
    serial_writeint(i);
    serial_writestring("\nstart= ");
    serial_writehex(free_memory_blocks[i].start);
    serial_writestring("\nend= ");
    serial_writehex(free_memory_blocks[i].end);
    serial_writestring("\n");
  }
}