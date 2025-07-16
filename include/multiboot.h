/**
 * @file  multiboot.h
 * @brief multiboot structures for information from bootloader
 *        from GNU Website
 * @date 2025-07-14
 */

#ifndef MULTIBOOT_H
#define MULTIBOOT_H

#include <stdint.h>

#define MAGIC 0x2BADB002

#define CHECK_FLAG(flags, bit) ((flags) & (1 << (bit)))

extern uint8_t __kernel_start;
extern uint8_t __kernel_end;

#define KERNEL_START ((uintptr_t)&__kernel_start)
#define KERNEL_END   ((uintptr_t)&__kernel_end)

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


#endif