#include "zone.h"
#include "list.h"
#include "serial.h"
#include "mm.h"

extern zone_t zone;
void test_buddy(zone_t *zone);

void free_buddy_system(zone_t* zone)
{
    int iCnt = 0;
    struct list_head *temp = NULL;
    for(iCnt = 0; iCnt < BUDDY_GROUPS; iCnt++){
        list_for_each_del(temp, &zone->free_area[iCnt].free_list){
            list_del(temp); 
        }
        INIT_LIST_HEAD(&zone->free_area[iCnt].free_list);
    }
}

void show_buddy(zone_t *zone)
{
    free_area_t *free_area = zone->free_area;

    printk("----------------------showing buddy---------------------------\n");
    int iCnt = 0;
    for(iCnt = 0; iCnt < BUDDY_GROUPS; iCnt++){
        if(list_is_empty(&free_area[iCnt].free_list) ){
            printk("order  is empty : %d\n", iCnt);
        }
        else{
            printk("order  is not empty : %d\n", iCnt);

            struct page *tmp = NULL;
            int bCnt = 0;
            list_for_each_entry(tmp, &free_area[iCnt].free_list, lru){
                printk(" bCnt : %d, page_no : %d\n", bCnt++, tmp->page_no);
            }
        }
    }
}

void ClearPagePrivate(struct page *page)
{
    page->flags &= ~PG_FLAG_private;
}

void ClearPageSlab(struct page *page)
{
    page->flags &= ~PG_FLAG_slab;
}

void SetPagePrivate(struct page *page)
{
    page->flags |= PG_FLAG_private;
}

void SetPageSlab(struct page *page)
{
    page->flags |= PG_FLAG_slab;
}

struct page *allocate_block(zone_t *zone, short order)
{
    short current_order = 0;
    free_area_t *area = NULL;
    int size = 0;
    struct page *page = NULL, *buddy = NULL;
    
    for(current_order = order; current_order < BUDDY_GROUPS; current_order++){
        area = zone->free_area + current_order;
        if(!list_is_empty(&zone->free_area[current_order].free_list) ){
            goto block_found;
        }
    }
    return NULL;

block_found:
    page = list_entry(area->free_list.next, struct page, lru);
    list_del(&page->lru); 

    ClearPagePrivate(page);
    page->private = 0;
    area->nr_free--;
    zone->free_pages -= 1UL << order;

    size = 1 << current_order;
    while(current_order > order){
        area--;
        current_order--;
        size >>= 1;
        buddy = page + size;

        list_add(&area->free_list, &buddy->lru);
        area->nr_free++;
        buddy->private = current_order;
        SetPagePrivate(buddy);
    }
    return page;
}

struct page *alloc_pages(int flags, short order)
{
    return allocate_block(&zone, order);
}

int PagePrivate(struct page *page)
{
    if((page->flags & PG_FLAG_private) == PG_FLAG_private){
        return 1;
    }
    return 0;
}

static inline int page_is_buddy(struct page *buddy, short order)
{
    if(PagePrivate(buddy) && buddy->private == order) {
        return 1;
    }
    return 0;
}

void free_block(struct page *page, zone_t *zone, short order)
{
    struct page *base = zone->zone_mem_map;
    unsigned long buddy_idx, page_idx = page - base;
    struct page *buddy = NULL, *coalesced = NULL;
    int order_size = 1 << order;

    zone->free_pages += order_size;

    while (order < BUDDY_GROUPS - 1){
        buddy_idx = page_idx ^ (1 << order);
        
        // Check if buddy is within zone bounds
        if (buddy_idx >= zone->present_pages) {
            break;
        }
        
        buddy = base + buddy_idx;
        
        if(!page_is_buddy(buddy, order)){
            break;
        }
        
        list_del(&buddy->lru);
        zone->free_area[order].nr_free--;
        ClearPagePrivate(buddy);
        buddy->private = 0;
        page_idx &= buddy_idx;
        order++;
    }

    coalesced = base + page_idx;
    coalesced->private = order;
    SetPagePrivate(coalesced);
    list_add(&zone->free_area[order].free_list, &coalesced->lru);
    zone->free_area[order].nr_free++;
}

void free_pages(struct page *page, short order)
{
    free_block(page, page->zone, order);
}

int order(int num)
{
    int iOrder = 0;
    if (num == 0)
        return -1;
    while (num > 1) {
        num >>= 1;
        iOrder++;
    }
    return iOrder;
}

void add_free_region_to_buddy(zone_t *zone, int start, int end)
{
    int current = start;
    
    printk("Adding free region from %d to %d\n", start, end);
    
    while (current <= end) {
        printk("current is %d\n", current);
        int remaining = end - current + 1;
        int max_order = 0;
        
        // Find the largest aligned block that fits
        // Use current position's natural alignment
        int alignment = current ? __builtin_ctz(current) : BUDDY_GROUPS;
        printk("alignment is %d\n", alignment);
        
        // Find order needed for remaining size
        printk("remaining is %d\n", remaining);
        int size_order = 0;
        int temp = remaining;
        while (temp > 1) {
            temp >>= 1;
            size_order++;
        }
         printk("size_order is %d\n", size_order);
       
        // Take minimum of alignment and size
        max_order = (alignment < size_order) ? alignment : size_order;
        if (max_order >= BUDDY_GROUPS) {
            max_order = BUDDY_GROUPS - 1;
        }
        
        int block_size = 1 << max_order;

        printk(" Page: %d, order: %d, ( %d pages ) \n", current, max_order, block_size);
        
        free_block(&zone->zone_mem_map[current], zone, max_order);
        current += block_size;
    }
}

void init_zone(zone_t *zone)
{
    int iCnt = 0, iPrev = 0;
    struct page *page = NULL;
    
    // Initialize all pages
    for(iCnt = 0; iCnt < zone->present_pages; iCnt++){
        zone->zone_mem_map[iCnt].page_no = iCnt;
        zone->zone_mem_map[iCnt].zone = zone;
        INIT_LIST_HEAD(&zone->zone_mem_map[iCnt].lru);
    }
    
    page = zone->zone_mem_map;
    iCnt = 0;
    
    // Skip initial taken/reserved pages
    while (iCnt < zone->present_pages && 
           (IS_FLAG(page[iCnt].flags, PG_FLAG_TAKEN) || 
            IS_FLAG(page[iCnt].flags, PG_FLAG_RESERVED))) {
        iCnt++;
    }
    iPrev = iCnt;
    
    printk("\nFirst free page starts at: %d\n", iPrev);
    
    // Scan through memory and add free regions
    while (iCnt < zone->present_pages) {
        // Found a taken/reserved page?
        if (IS_FLAG(page[iCnt].flags, PG_FLAG_TAKEN) || 
            IS_FLAG(page[iCnt].flags, PG_FLAG_RESERVED)) {
            
            // Add the free region [iPrev, iCnt-1] to buddy system
            if (iPrev < iCnt) {
                add_free_region_to_buddy(zone, iPrev, iCnt - 1);
            }
            
            // Skip over taken/reserved pages
            while (iCnt < zone->present_pages && 
                   (IS_FLAG(page[iCnt].flags, PG_FLAG_TAKEN) || 
                    IS_FLAG(page[iCnt].flags, PG_FLAG_RESERVED))) {
                iCnt++;
            }
            iPrev = iCnt;
        } else {
            iCnt++;
        }
    }
    
    // Add final free region if any
    if (iPrev < zone->present_pages) {
        printk("Adding final free region\n");
        add_free_region_to_buddy(zone, iPrev, zone->present_pages - 1);
    }
    
    printk("\nBuddy allocator initialization complete\n");
    test_buddy(zone);
}

void test_buddy(zone_t *zone)
{
    show_buddy(zone);
    
    printk("\n=== Allocating order 9 (512 pages) ===\n");
    struct page *page1 = allocate_block(zone, 9);
    if (page1) {
        printk("Allocated page: ", page1->page_no);
    }
    show_buddy(zone);

    printk("\n=== Allocating order 4 (16 pages) ===\n");
    struct page *page2 = allocate_block(zone, 4);
    if (page2) {
        printk("Allocated page: ", page2->page_no);
    }
    show_buddy(zone);

    printk("\n=== Freeing order 4 ===\n");
    free_block(page2, zone, 4);
    show_buddy(zone);

    printk("\n=== Freeing order 9 ===\n");
    free_block(page1, zone, 9);
    show_buddy(zone);
}
