#ifndef KERNEL_H
#define KERNEL_H

#include <stdint.h>

#define OFFSET1 0x20
#define OFFSET2 0x28
#define FREQUENCY 100
// #define MAGIC 0x2BADB002
#define MAX_MEMORY_BLOCKS 32

#define CHECK_FLAG(flags, bit) ((flags) & (1 << (bit)))

extern uint8_t __kernel_start;
extern uint8_t __kernel_end;

typedef struct multiboot_info {
  uint32_t flags;

  uint32_t mem_lower;
  uint32_t mem_upper;

  uint32_t boot_device;

  uint32_t cmdline;

  uint32_t mods_count;
  uint32_t mods_addr;

  union {
    struct {
      uint32_t tabsize, strsize, addr, pad;
    } aout_sym;
    struct {
      uint32_t num, size, addr, shndx;
    } elf_sec;
  } u;

  uint32_t mmap_length;
  uint32_t mmap_addr;
} __attribute__((packed)) multiboot_info_t;

typedef struct multiboot_mmap_entry {
  uint32_t size;
  uint64_t addr;
  uint64_t len;
  uint32_t type;
} __attribute__((packed)) multiboot_mmap_entry_t;

typedef struct memory_blocks {
  uint64_t start;
  uint64_t end;
} __attribute__((packed)) memory_blocks_t;

memory_blocks_t free_memory_blocks[MAX_MEMORY_BLOCKS];

int find_available_memory(multiboot_info_t *mbi);
void print_blocks(int num_blocks);
void kernel_main(uint32_t magic, uint32_t addr);

#endif