#include <stddef.h>
#include <stdint.h>

#include "list.h"
#include "serial.h"
#include "utils.h"
#include "zone.h"
#include "multiboot.h"
#include "mm.h"
#include "manager.h"
#include "allocator.h"

struct page *mem_map = NULL;
unsigned long swapper_pg_dir[PG_DIR_ENTRIES] __attribute__((aligned(4096)));

struct phy_layout phy_layout;


void memset(void *addr, char val, unsigned int size)
{
    char *ptr = (char*)addr;
    register int iCnt = 0;
    while (iCnt < size){
        ptr[iCnt++]=val;
    }
}


void print_machine_map(void)
{
    serial_writestring("\n highest_usable_pfn: 0x");
    serial_writehex(phy_layout.highest_usable_pfn);
    serial_writestring("\n total_ram_pages: 0x");
    serial_writehex(phy_layout.totalram_pages);
    serial_writestring("\n max_pfn: 0x");
    serial_writehex(phy_layout.max_pfn);
    serial_writestring("\n max_low_pfn: 0x");
    serial_writehex(phy_layout.max_low_pfn);
    serial_writestring("\n high_start_pfn: 0x");
    serial_writehex(phy_layout.highstart_pfn);
    serial_writestring("\n high_end_pfn: 0x");
    serial_writehex(phy_layout.highend_pfn);
    serial_writestring("\n total_high_pages: 0x");
    serial_writehex(phy_layout.totalhigh_pages);

}

void machine_specific_memory_setup(multiboot_info_t *mbi, memory_blocks_t* blocks, int num_blocks)
{
    unsigned long total_ram_bytes = (mbi->mem_upper * 1024 + (1024 * 1024));  // mem_upper in KB
    unsigned long total_ram_pfn   = total_ram_bytes / PAGE_SIZE;
    unsigned long lowmem_limit_pfn = ((mbi->mem_lower*1024) / PAGE_SIZE);

    phy_layout.highest_usable_pfn = ( mbi->mem_upper*1024 ) / PAGE_SIZE;
    phy_layout.totalram_pages = total_ram_pfn;
    phy_layout.max_pfn = phy_layout.highest_usable_pfn;
    // pick the smaller one, karan ram 896mb peksha kami asel tar
    if (total_ram_pfn < lowmem_limit_pfn)
        phy_layout.max_low_pfn = total_ram_pfn;
    else
        phy_layout.max_low_pfn = lowmem_limit_pfn;

    phy_layout.highstart_pfn = phy_layout.max_low_pfn;
    phy_layout.highend_pfn = phy_layout.max_pfn;
    phy_layout.totalhigh_pages = phy_layout.highend_pfn - phy_layout.highstart_pfn;

    print_machine_map();

}

void init_page(struct page* page)
{
    memset(page, 0, sizeof(struct page));
}

void init_zone(zone_t *zone)
{
    zone->present_pages = phy_layout.totalram_pages;
    /* zone->free_pages = PAGING_PAGES; */
    zone->zone_mem_map = mem_map;
    zone->pages_min = ZONE_WATERMARK;
    zone->pages_low = ZONE_WATERMARK;
    memset(zone->zone_mem_map, 0, phy_layout.totalram_pages*sizeof(struct page));
}
/*
 * this function calculates the number of 
 * page tables needed to map the whole memory
 */
unsigned int count_pgdir_entries(zone_t *zone)
{
    unsigned long zone_mem = zone->present_pages * PAGE_SIZE;
    unsigned int pgtbl_size = PAGE_SIZE * PG_TABLE_ENTRIES;

    return zone_mem / pgtbl_size;
}

unsigned long get_free_page(void)
{
    register int iCnt = 0;
    for (iCnt = 0; iCnt < phy_layout.totalram_pages; iCnt++){
        if(!(mem_map[iCnt].flags & PG_FLAG_TAKEN)){
            mem_map[iCnt].flags |= PG_FLAG_TAKEN;
            return (unsigned long)iCnt * PAGE_SIZE;
        }
    }
    return 0;
}

void print_mem_map(void)
{
    register int iCnt = 0;
    for (iCnt = 0; iCnt < phy_layout.totalram_pages; iCnt++){
        serial_writestring("\n page : ");
        serial_writehex(iCnt);
        serial_writestring("\tstatus: ");
        if((mem_map[iCnt].flags & PG_FLAG_RESERVED )== PG_FLAG_RESERVED){
             serial_writestring("reserved, ");
        }
         if((mem_map[iCnt].flags & PG_FLAG_TAKEN) == PG_FLAG_TAKEN){
             serial_writestring("taken, ");
        }       
        else{
         serial_writestring("free, ");

        }

    }
}

int create_mem_map(multiboot_info_t *mbi)
{
    register unsigned int iCnt = 0;
    unsigned long phy_start = 0, phy_end  = 0;
    unsigned int page_start = 0, page_end = 0;
    mem_map = (struct page*)kalloc(phy_layout.totalram_pages * sizeof(struct page));
    if(!mem_map){
        serial_writestring("fail mem_map allocation\n");
        return 0;
    }

    memset(mem_map, 0, phy_layout.totalram_pages * sizeof(struct page));
    serial_writestring("\nmem_map starts at addr :");
    serial_writehex(mem_map);

    for (iCnt = 0; iCnt < phy_layout.totalram_pages; iCnt++) {
        mem_map[iCnt].flags = (PG_FLAG_TAKEN | PG_FLAG_RESERVED);
    }


    
    /*
     * mem_map's data is present in the lower 640kb . Hence we don't want to wipe it off.
     * Mark the whole 640kb of pages as PG_TAKEN , since you don't want to replace the mem_map data 
     * structure. Once initialzed copy the mem_map to some_where up via page_allocator and free the existing pages 
     * in lower 640kb memory
     */
    for(iCnt = 0; iCnt < MAX_MEMORY_BLOCKS; iCnt++){
        if(free_memory_blocks[iCnt].start==0 || free_memory_blocks[iCnt].end==0)
            continue;
        
        phy_start = free_memory_blocks[iCnt].start ;
        serial_writestring("\ncurr aligned phy_start: \t");
        serial_writehex(phy_start);

        if(!IS_PAGE_ALIGNED(phy_start)){
            phy_start = ALIGN_PAGE(phy_start) + PAGE_SIZE;
        }
        serial_writestring("\nnew aligned phy_start: \t");
        serial_writehex(phy_start);

        page_start = phy_start/PAGE_SIZE;
        serial_writestring("\nnew aligned page_start: \t");
        serial_writehex(page_start);

        /*
         * For example free_block end is 4120 
         * so 0 - 4095 is free but , for next page 4096 - 8191
         * only till 4120 is free, hence avoid that page, 
         * page_end will the the last free_page */

        phy_end = free_memory_blocks[iCnt].end;
        serial_writestring("\ncurr aligned phy_end: \t");
        serial_writehex(phy_end);

        if(!IS_PAGE_ALIGNED(phy_end)){
            phy_end = ALIGN_PAGE(phy_end) + PAGE_SIZE - 1;
        }
        serial_writestring("\nnew aligned phy_end: \t");
        serial_writehex(phy_end);

        page_end = (phy_end-1)/PAGE_SIZE;
        serial_writestring("\nnew aligned page_end: \t");
        serial_writehex(page_end);

        for( iCnt = page_start; iCnt <= page_end; iCnt++ ){
            mem_map[iCnt].flags &= ~PG_FLAG_TAKEN;
            mem_map[iCnt].flags &= ~PG_FLAG_RESERVED;

        }

    }

    print_mem_map();
    return 1;
}

unsigned int pgd_index(unsigned int phy_addr)
{
    return (phy_addr/(PG_DIR_ENTRIES*PAGE_SIZE));
}
/*
 * The pages containing page tables start at 1mb */
void setup_paging(zone_t *zone, unsigned int pgdir_entries)
{
    register int iCnt = 0, jCnt = 0;     
    unsigned long phy_addr = 0x00000000;
    unsigned int pgd = pgd_index((unsigned int)PAGE_OFFSET);
    unsigned long temp_addr = 0;
    unsigned long pg_ptr;
    
    for (iCnt = 0; iCnt < pgdir_entries; iCnt++){
        temp_addr = get_free_page();
        swapper_pg_dir[pgd+iCnt] = temp_addr;
        pg_ptr = (temp_addr & 0xFFFFF000) | PG_PRESENT | PG_RW;
        jCnt = 0;
        while (phy_addr < (phy_layout.max_low_pfn * PAGE_SIZE)){
            /* (unsigned long*)pg_ptr[jCnt++] = (phy_addr & 0xFFFFF000) | PG_PRESENT | PG_RW ; */
            phy_addr += PAGE_SIZE;
        }
    }
    enable_paging((unsigned long)swapper_pg_dir);
}

void init_mem(multiboot_info_t *mbi)
{
    register int iCnt = 0, jCnt = 0; 
    unsigned int pgdir_entries = 0;
    int iRet = 0;
    zone_t zone;

    iRet = create_mem_map(mbi);
   
    /* init_zone(&zone); */
   
    /* pgdir_entries = count_pgdir_entries(&zone); */
    /* setup_paging(&zone, pgdir_entries); */
    if(!iRet){
        serial_writestring("create mem_map fail\n");
    }
    
}

