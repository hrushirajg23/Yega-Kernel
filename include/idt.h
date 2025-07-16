/**
 * @file  idt.h
 * @brief Setting up Interrupt Descriptor Table
 * @date 2025-07-14
 */

#ifndef IDT_H
#define IDT_H

#include <stdint.h>

/* IDT entry struct */
typedef struct {
  uint16_t isr_low;
  uint16_t kernel_cs;
  uint8_t reserved;
  uint8_t attributes;
  uint16_t isr_high;
} __attribute__((packed)) idt_entry_t;

/* IDT pointer struct */
typedef struct {
  uint16_t limit;
  uint32_t base;
} __attribute__((packed)) idtr_t;

void setup_idt(void);
void initialize_idt(void);

#endif