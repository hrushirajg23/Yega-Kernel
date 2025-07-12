#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "gdt.h"
#include "idt.h"
#include "irq.h"
#include "kernel.h"
#include "pic.h"
#include "pit.h"
#include "serial.h"
#include "vga_display.h"

#if defined(__linux__)
#error                                                                         \
    "You are not using a cross-compiler, you will most certainly run into trouble"
#endif

#if !defined(__i386__)
#error "This tutorial needs to be compiled with a ix86-elf compiler"
#endif
#define MAGIC 0x2BADB002
uintptr_t kernel_start = (uintptr_t)&__kernel_start;
uintptr_t kernel_end = (uintptr_t)&__kernel_end;

int find_available_memory(multiboot_info_t *mbi) {
  serial_writestring("mmap addr= ");
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

      if (entry_start <= kernel_end && entry_end > kernel_end) {
        free_memory_blocks[num_block].start = kernel_end;
        free_memory_blocks[num_block].end = entry_end;
        num_block++;
      } else if (entry_start > kernel_end) {
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

void kernel_main(uint32_t magic, uint32_t addr) {

  if (magic != 0x2BADB002) {
    // Bootloader not Multiboot-compliant
    while (1)
      asm volatile("hlt");
  }

  int num_blocks;

  multiboot_info_t *mbi = (multiboot_info_t *)addr;

  serial_init();
  serial_writestring("Serial initialized. Booting...\n");

  serial_writestring("kernel_start= ");
  serial_writehex(kernel_start);

  serial_writestring("\nkernel_end= ");
  serial_writehex(kernel_end);

  num_blocks = find_available_memory(mbi);
  if (num_blocks == 0) {
    serial_writestring("\nNo memory available.\n");
    while (1)
      asm volatile("hlt");
  } else {
    serial_writestring("\nMemory available.\n");
    print_blocks(num_blocks);
  }

  serial_writestring("flags= ");
  serial_writehex(mbi->flags);

  terminal_initialize();
  terminal_writestring("Hello, Welcome To Yega Kernel!\n");

  serial_writestring("\nGDT init...\n");
  gdt_initialize();

  serial_writestring("PIC remap...\n");
  remap_pic(OFFSET1, OFFSET2);

  serial_writestring("IDT setup...\n");
  setup_idt();

  serial_writestring("Install timer & keyboard drivers..\n");
  install_handlers();

  serial_writestring("PIT init...\n");
  init_timer(FREQUENCY);

  serial_writestring("IDT init...\n");
  initialize_idt();

  serial_writestring("Boot complete.\n");

  while (1) {
    asm volatile("hlt");
  }
}