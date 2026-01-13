/**
 * @file  kernel.h
 * @brief Functionality for kernel main entry
 * @date 2025-07-14
 */

#ifndef KERNEL_H
#define KERNEL_H

#include <stdint.h>

#define OFFSET1 0x20
#define OFFSET2 0x28
#define FREQUENCY 100

/* Below macros are used for hardware cache alignment
 */

#define ALIGN(x, a)		__ALIGN_KERNEL((x), (a))
#define __ALIGN_KERNEL(x, a)		__ALIGN_KERNEL_MASK(x, (typeof(x))(a) - 1)
#define __ALIGN_KERNEL_MASK(x, mask)	(((x) + (mask)) & ~(mask))

#define CONFIG_X86_L1_CACHE_SHIFT 5 
#define L1_CACHE_SHIFT	(CONFIG_X86_L1_CACHE_SHIFT)
#define L1_CACHE_BYTES	(1 << L1_CACHE_SHIFT)

#define cache_line_size() L1_CACHE_BYTES
#define ULONG_MAX	(~0UL)

#define ____cacheline_aligned \
    __attribute__((__aligned__(L1_CACHE_BYTES)))

void kernel_main(uint32_t magic, uint32_t addr);

#endif
