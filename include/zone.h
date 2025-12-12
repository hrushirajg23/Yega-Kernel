#ifndef _ZONE_H
#define _ZONE_H

#include "list.h"
#include "manager.h"
#include "utils.h"

/* Assuming we have only 1024kb memory and divide the memory into
 * 4 groups for now. 2^n where 0<=n<4. N is number of pages combined
 * to form a group.
 */


/*
 * order    no of pages     size
 * 0        1               4kb
 * 1        2               8kb
 * 2        4               16kb
 * 3        8               32kb
 * 4        16              64kb
 * 5        32              128kb
 * 6        64              256kb
 * 7        128             512kb
 * 8        256             1024kb (1mb)
 * 9        512             2048kb (2mb)
 * 10       1024            4096kb (4mb)
 *  
 * -----------------
 */

#define MEMORY_POOL 8*1024*1024
#define BUDDY_GROUPS 11
#define PG_DIR_ENTRIES 1024
#define PG_TABLE_ENTRIES 1024
#define PAGE_SIZE 4096

/* Software flags */
#define PG_FLAG_LOCKED 1<<0
#define PG_FLAG_referenced 1<<1
#define PG_FLAG_dirty 1<<2
#define PG_FLAG_lru 1<<3
#define PG_FLAG_private 1<<4
#define PG_FLAG_reclaim 1<<5
#define PG_FLAG_TAKEN 1<<6
#define PG_FLAG_RESERVED 1<<7
#define PG_FLAG_slab 1<<8

/* Hardware flags */
#define PG_PRESENT 1<<0
#define PG_RW 1<<1 //if not set , the page is read only
#define PG_USER 1<<2 //if set, everyone can access the page
#define PG_ACCESSED 1<<5
#define PG_DIRTY 1<<6 //set when page has been written to

#define PAGE_OFFSET 0xC0000000
#define ZONE_HIGH_MEM 0x38000000  // 896MB
#define PAGE_SHIFT 12
                                    
#define ZONE_WATERMARK 10
typedef unsigned long phys_addr_t;

struct zone;
struct page{
    struct zone *zone;
    unsigned int page_no; //for testing
    unsigned int flags;
    unsigned int ref_cnt;
    unsigned int _map_count; // number of page table entries that refer to the page 
    unsigned int private; //use by buddy to keep order 
    struct list_head lru;
    struct page *first_page; //compound page i.e pointer to the first page in group when allocated to a slab
};

typedef struct {
    struct list_head free_list;
    unsigned long nr_free; //number of free groups in the order lane
}free_area_t;

typedef struct zone {
    unsigned long free_pages; 
    unsigned long present_pages;
    unsigned long pages_min; //min no of pages to be reserved
    unsigned long pages_low; //watermark to initiate the page fram reclaiming algo
    unsigned long zone_start_pfn; //index of the first page frame of the zone
    struct page* zone_mem_map;
    free_area_t free_area[BUDDY_GROUPS];
    char *name;
}zone_t;

struct phy_layout {
    unsigned int highest_usable_pfn; //page frame number of highest usabled page frame
    unsigned int totalram_pages; //total number of usable page frames
    unsigned int min_low_pfn; //page frame number of the first usable pagr frame after kernel image in RAM
    unsigned int max_pfn; //page frame number of last usable page frame
    unsigned int max_low_pfn; //Page frame number of the last page frame directly mapped by the kernel (low memory)
    unsigned int totalhigh_pages; //Total number of page frames not directly mapped by the kernel (high memory)
    unsigned int highstart_pfn; //Page frame number of the first page frame not directly mapped by the kernel
    unsigned int highend_pfn; //Page frame number of the last page frame not directly mapped by the kernel
};

void machine_specific_memory_setup(multiboot_info_t *mbi, memory_blocks_t* blocks, int num_blocks);
void enable_paging(unsigned long int);
unsigned long get_free_page(void);
void init_zone(zone_t *);
struct page *virt_to_page(void *);
struct page *alloc_pages(int , short );
void free_pages(struct page *, short );
void ClearPageSlab(struct page *);
void SetPageSlab(struct page *);
unsigned long page_to_phys(struct page *);
void show_buddy(zone_t *zone);
void print_mem_map(void);




#define ZONE_DMA 1 << 29
#define ZONE_NORMAL 1 << 30
#define ZONE_HIGHMEM 1 << 31


#endif
