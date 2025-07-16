/**
 * @file  registers.h
 * @brief Structure for registers
 * @date 2025-07-14
 */

#ifndef REGISTERS_H
#define REGISTERS_H

#include <stdint.h>

typedef struct {
  uint32_t ds, es, fs, gs;
  uint32_t edi, esi, ebp, esp;
  uint32_t ebx, edx, ecx, eax;
  uint32_t int_no, err_code;
} registers_t;

#endif