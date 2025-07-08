#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "idt.h"

#define IDT_MAX_DESCRIPTORS 256
#define KERNEL_CS 0x08

__attribute__((aligned(0x10))) static idt_entry_t idt[IDT_MAX_DESCRIPTORS];
static idtr_t idtr;
extern void (*isr_stub_table[IDT_MAX_DESCRIPTORS])(void);
extern void (*irq_stub_table[IDT_MAX_DESCRIPTORS])(void);

static void load_idt(idtr_t *idt_descriptor) {
  __asm__ volatile("lidt (%0)" : : "r"(idt_descriptor) : "memory");
  __asm__ volatile("sti");
}

static void set_idt_entry(uint8_t vector, void (*isr)(void), uint8_t flags) {
  idt[vector].isr_low = (uint32_t)isr & 0xFFFF;
  idt[vector].kernel_cs = KERNEL_CS;
  idt[vector].reserved = 0;
  idt[vector].attributes = flags;
  idt[vector].isr_high = (uint32_t)isr >> 16;
}

void setup_idt(void) {
  for (uint8_t vector = 0; vector < 32; vector++) {
    set_idt_entry(vector, isr_stub_table[vector], 0x8E);
  }

  for (uint8_t vector = 32; vector < 48; vector++) {
    set_idt_entry(vector, irq_stub_table[vector - 32], 0x8E);
  }
}

void initialize_idt() {
  idtr.limit = sizeof(idt) - 1;
  idtr.base = (uint32_t)idt;

  load_idt(&idtr);
}