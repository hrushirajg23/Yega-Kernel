#include "zone.h"
#include "list.h"
#include "serial.h"
#include "mm.h"


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

    serial_writestring("----------------------showing buddy---------------------------\n");
    int iCnt = 0;
    for(iCnt = 0; iCnt < BUDDY_GROUPS; iCnt++){
        if(list_is_empty(&free_area[iCnt].free_list) ){
            serial_writestring("order  is empty : ");
            serial_writehex(iCnt);
            serial_writestring("\n");
        }
        else{
            serial_writestring("order  is not empty : ");
            serial_writedec(iCnt);
            serial_writestring("\n");

            struct page *tmp = NULL;
            int bCnt = 0;
            list_for_each_entry(tmp, &free_area[iCnt].free_list, lru){
                serial_writestring(" bCnt : ");
                serial_writedec(bCnt++);
                serial_writestring(" page_no: ");
                serial_writedec(tmp->page_no);
                serial_writestring(" \n");

            }

        }
    }
}


void ClearPagePrivate(struct page *page)
{
    //clears the page flag associated with the page
    //the flag is PG_private
    page->flags &= ~PG_FLAG_private;
}

void SetPagePrivate(struct page *page)
{
    page->flags |= PG_FLAG_private;
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

    /* printf("got block at order %hi\n", current_order); */
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

int PagePrivate(struct page *page)
{
    if((page->flags & PG_FLAG_private) == PG_FLAG_private){
        return 1;
    }
    return 0;
}

static inline int page_is_buddy(struct page *buddy, short order)
{
    if(PagePrivate(buddy) && buddy->private == order){
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

    /* printf(" page_idx = %lu\n", page_idx); */
    zone->free_pages += order_size;

    while (order < 10){
        buddy_idx = page_idx ^ (1 << order);
        buddy = base + buddy_idx;
        if(!page_is_buddy(buddy, order)){
            break;
        }
        serial_writestring(" found buddy at page ");
       serial_writedec( buddy->page_no);
       serial_writestring("\n");
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

int order (int num)
{
    int iOrder = 0;
    if ( num == 0 )
        return -1;
    while ( num > 1 )
    {
        num >>= 1;
        iOrder++;
    }
    return iOrder;
}

int fit_to_possible(int diff)
{
    register int iFit = 1 << (BUDDY_GROUPS-1);
    while (iFit > diff){
        iFit >>= 1;
    }
    return iFit;
}

void break_block (struct page *prev, struct page *curr, zone_t *zone)
{
    int diff = (curr - prev)  + 1; //diff bet pages
    int iFit = fit_to_possible(diff);

    if (iFit != diff){
        serial_writestring(" iFit != diff \n iFit: ");
        serial_writedec(iFit);
        serial_writestring("diff: ");
        serial_writedec(diff);
        serial_writestring("\n");
        serial_writestring("prev : ");
        serial_writehex(prev);
        serial_writestring("\n");
        serial_writestring("curr: ");
        serial_writehex(curr);
        serial_writestring("\n");
        break_block(prev + iFit, curr, zone);
        break_block(prev, prev + iFit - 1, zone);
        return;
    }
        serial_writestring(" iFit = diff \n iFit: ");
        serial_writedec(iFit);
        serial_writestring("diff: ");
        serial_writedec(diff);
        serial_writestring("\n");
        serial_writestring("prev : ");
        serial_writehex(prev);
        serial_writestring("\n");
        serial_writestring("curr: ");
        serial_writehex(curr);

        free_block(prev , zone, order(iFit)-1);
}

void test_buddy(zone_t *zone)
{
    show_buddy(zone);
    struct page *page1 = allocate_block(zone, 8);

    serial_writestring("showing buddy after allocate_block 1 \n");
    show_buddy(zone);

    struct page *page2 = allocate_block(zone, 4);
    serial_writestring("showing buddy after allocate_block 2 \n");
    show_buddy(zone);

    free_block(page2, zone, 4);
    serial_writestring("showing buddy after free_block 2 \n");
    show_buddy(zone);


    free_block(page1, zone, 8);
    serial_writestring("showing buddy after free_block 1 \n");
    show_buddy(zone);
}


void init_zone(zone_t *zone)
{
    register int iCnt = 0, iPrev = 0; 
    int largest_block_page_cnt = 0;
    struct page *page = NULL;

    largest_block_page_cnt = 1 << (BUDDY_GROUPS-1);
     
    for(iCnt = 0; iCnt < zone->present_pages; iCnt++){
        zone->zone_mem_map[iCnt].page_no = iCnt; 
        INIT_LIST_HEAD(&zone->zone_mem_map[iCnt].lru);
    }
    page = zone->zone_mem_map;
    iCnt = 0;
    while ( iCnt < zone->present_pages && 
                (IS_FLAG(page[iCnt].flags, PG_FLAG_TAKEN) || 
                 IS_FLAG(page[iCnt].flags, PG_FLAG_RESERVED ) )){
                iPrev++;
                iCnt++;
            }
    serial_writestring("\niPrev starts from : ");
    serial_writedec(iPrev);
    while ( iCnt < zone->present_pages ) {
        if ( IS_FLAG(page[iCnt].flags, PG_FLAG_TAKEN) || 
                IS_FLAG(page[iCnt].flags, PG_FLAG_RESERVED ) ){
            serial_writestring("\n found reserved block at  : ");
            serial_writedec(iCnt);

            break_block(zone->zone_mem_map + iPrev, zone->zone_mem_map + (iCnt -1), zone);
            while ( iCnt < zone->present_pages && 
                (IS_FLAG(page[iCnt].flags, PG_FLAG_TAKEN) || 
                 IS_FLAG(page[iCnt].flags, PG_FLAG_RESERVED ) )){
                iCnt++;
            }
            iPrev = iCnt;
        }
        else if ( (iCnt-iPrev!=0) && ((iCnt - iPrev) % largest_block_page_cnt) == 0 ){
            serial_writestring("\n completed largest block from : ");
            serial_writedec(iPrev);
            serial_writestring(" till : ");
            serial_writedec(iCnt-1);
            serial_writestring("\n");
            break_block(zone->zone_mem_map + iPrev, zone->zone_mem_map + ( iCnt - 1 ), zone);
            iPrev = iCnt;
        }
        iCnt++;
    }

    test_buddy(zone);
}



