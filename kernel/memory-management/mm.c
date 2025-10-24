#include <stddef.h>
#include <stdint.h>

#include "list.h"
#include "serial.h"
#include "utils.h"
#include "zone.h"
#include "multiboot.h"


struct page mem_map[PAGING_PAGES] = {0, };
unsigned long swapper_pg_dir[PG_DIR_ENTRIES] __attribute__((aligned(4096)));

struct phy_layout phy_layout;

inline unsigned int pgd_index(unsigned int phy_addr)
{
    return (phy_addr/(PG_DIR_ENTRIES*PAGE_SIZE));
}

void machine_specific_memory_setup(multiboot_info_t *mbi)
{
    unsigned long total_ram_bytes = mbi->mem_upper * 1024;  // mem_upper in KB
    unsigned long total_ram_pfn   = total_ram_bytes / PAGE_SIZE;
    unsigned long lowmem_limit_pfn = (LOW_MEM_LIMIT / PAGE_SIZE);
    phy_layout.highest_usable_pfn = ( mbi->mem_upper*1024 + 0x10000) / PAGE_SIZE;
    phy_layout.max_pfn = phy_layout.highest_usable_pfn;
    // pick the smaller one, karan ram 896mb peksha kami asel tar
    if (total_ram_pfn < lowmem_limit_pfn)
        phy_layout.max_low_pfn = total_ram_pfn;
    else
        phy_layout.max_low_pfn = lowmem_limit_pfn;

    phy_layout.highstart_pfn = phy_layout.max_low_pfn;
    phy_layout.highend_pfn = phy_layout.max_pfn;
    phy_layout.totalhigh_pages = phy_layout.highend_pfn - phy_layout.highstart_pfn;

    }

void init_page(struct page* page)
{
    memset(page, 0, sizeof(struct page));
}

void init_zone(zone_t *zone)
{
    zone.present_pages = PAGING_PAGES;
    zone.free_pages = PAGING_PAGES;
    zone->mem_map = mem_map;
    zone->pages_min = MIN_ZONE_PAGES;
    zone->pages_low = ZONE_WATERMARK;
    memset(zone->mem_map, 0, PAGING_PAGES*sizeof(struct page));
}
/*
 * this function calculates the number of 
 * page tables needed to map the whole memory
 */
unsigned int count_pgdir_entries(zone_t *zone)
{
    unsigned long zone_mem = zone->present_pages * PAGE_SIZE;
    unsigned int pgtbl_size = PAGE_SIZE * PG_TBL_ENTRIES;

    return zone_mem / pgtbl_size;
}

unsigned long get_free_page(void)
{
    register int iCnt = 0;
    for (iCnt = 0; iCnt < PAGING_PAGES; iCnt++){
        if(!(mem_map[iCnt].flags & PG_TAKEN)){
            mem_map[iCnt].flags |= PG_TAKEN;
            return (unsigned long)iCnt * PAGE_SIZE;
        }
    }
    return 0;
}

/*
 * The pages containing page tables start at 1mb */
void setup_paging(zone_t *zone, unsigned int pgdir_entries)
{
    register int iCnt = 0, jCnt = 0;     
    unsigned long phy_addr = 0x00000000;
    unsigned int pgd = pgd_index(PAGE_OFFSET);
    unsigned long temp_addr = 0;
    unsigned long *pg_ptr = NULL;
    
    for (iCnt = 0; iCnt < pgdir_entries; iCnt++){
        temp_addr = get_free_page();
        swapper_pg_dir[pgd+iCnt] = temp_addr;
        pg_ptr = (temp_addr & 0xFFFFF000) | PG_PRESENT | PG_RW;
        jCnt = 0;
        while (phy_addr < (phy_layout.max_low_pfn * PAGE_SIZE)){
            pg_ptr[jCnt++] = (phy_addr & 0xFFFFF000) | PG_PRESENT | PG_RW ;
            phy_addr += PAGE_SIZE;
        }
    }
    enable_paging((unsigned long)swapper_pg_dir);
}

void init_mem(void)
{
    register int iCnt = 0, jCnt = 0; 
    unsigned int pgdir_entries = 0;
    zone_t zone;
    init_zone(&zone);
   
    pgdir_entries = count_pgdir_entries(&zone);
    setup_paging();
    

}

