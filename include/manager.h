/**
 * @file  manager.h
 * @brief Functionality for memory manager such as finding available memory and
 *        setting up heap
 * @date 2025-07-14
 */

#ifndef MANAGER_H
#define MANAGER_H

#include <stddef.h>
#include <stdint.h>

#include "multiboot.h"

#define MAX_MEMORY_BLOCKS 10
#define PAGE_SIZE 4096
#define HEAP_SIZE (256 * 4096)

int initialize_memeory_manager(multiboot_info_t *mbi);
int find_available_memory(multiboot_info_t *mbi);
void print_blocks(int num_blocks);

#endif
