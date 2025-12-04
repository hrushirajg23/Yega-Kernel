#ifndef _PAGE_H
#define _PAGE_H

#include "zone.h"
#include "mm.h"

extern zone_t zone;
extern struct page *mem_map;

#define __phys_addr(x)       ((x) - PAGE_OFFSET)	

#define __pa(x)             __phys_addr((unsigned long)(x))
#define __va(x)             ((void *)((unsigned long)(x) + PAGE_OFFSET))

#define pfn_to_page(pfn) (mem_map + ((pfn) - zone.zone_start_pfn))

#define virt_to_page(kaddr)	pfn_to_page(__pa(kaddr) >> PAGE_SHIFT)
#define pfn_to_kaddr(pfn)      __va((pfn) << PAGE_SHIFT)

#define phy_to_virt(p) ((void *)((p) + PAGE_OFFSET))

#define phys_to_page(p) pfn_to_page(p >> PAGE_SHIFT)


#endif
