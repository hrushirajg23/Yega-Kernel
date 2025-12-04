#ifndef _MM_H
#define _MM_H

#include "zone.h"
#include "page.h"

#define ALIGN_PAGE(address) (address - (address % PAGE_SIZE))
#define IS_PAGE_ALIGNED(address) (((address) % PAGE_SIZE) == 0)

#define CLEAR_FLAG(var, flag) ((var &= ~(flag)))
#define SET_FLAG(var, flag) ((var |= (flag)))
#define IS_FLAG(var, flag) ((var & flag) == (flag)) 


void memset(void *addr, char val, unsigned int size);
void *memcpy(void *dest, void *src, unsigned int size);
void init_mem(multiboot_info_t *);


static inline struct page *compound_head(struct page *page)
{
	// if (unlikely(PageTail(page)))
		return page->first_page;
	// return page;
}

static inline struct page *virt_to_head_page(const void *x)
{
	struct page *page = virt_to_page(x);
	return compound_head(page);
}
#endif
