#include "ext2_balloc.h"
#include "fs.h"
#include "serial.h"
#include "string.h"
#include "mm.h"

/* External references to global data in fs.c */
extern bgdesc_t bgdt[1024];
extern ext2_super_block super;
extern uint16_t n_block_groups;

#define S_BLOCK_SIZE 1024
#define EXT2_BLK_SIZE 1024

/* Bitmap operations */

void set_bit(uint8_t *bitmap, uint16_t i)
{
    uint16_t byte = i / 8;
    uint8_t bit = i % 8;
    bitmap[byte] |= (1 << bit);
}

void unset_bit(uint8_t *bitmap, uint16_t i)
{
    uint16_t byte = i / 8;
    uint8_t bit = i % 8;
    bitmap[byte] ^= (1 << bit);
}

/* Find first free bit in bitmap */
uint32_t first_free_bm(uint8_t *bitmap, uint8_t allowed_start)
{
    uint32_t first_free_block;
    /* calculate ki block madhlya konty byte pasun
     * start karaycha ahe
     */
    uint8_t start_byte = allowed_start / 8;
    
    for (int j = start_byte; j < S_BLOCK_SIZE; j++) {
        uint8_t bmj = bitmap[j];
        if (bmj == 255) continue; // full...
        
        uint8_t match;
        for (int k = 0; k < 8; k++) {
            if (((bmj >> k) & 1) == 0) {
               match = k; 
               break; 
            }
        }
        uint16_t first_free_entry = j * 8 + match;
        if (first_free_entry < allowed_start) continue;
        return first_free_entry;
    }
    return 0;
}


/* Find first free block in block group */
uint32_t first_free_block(bgdesc_t *d)
{
    uint8_t bitmap[S_BLOCK_SIZE];
    disk_read_blk(d->bg_block_bitmap, bitmap);
    return first_free_bm(bitmap, 0);
}

/* Find first free inode in block group */
uint32_t first_free_inode(bgdesc_t *d)
{
    uint8_t bitmap[S_BLOCK_SIZE];
    disk_read_blk(d->bg_inode_bitmap, bitmap);
    return first_free_bm(bitmap, EXT2_FREE_INO_START);
}

/* Find first free block across all block groups */
int first_free_block_num(uint32_t *res)
{
    for (int i = 0; i < n_block_groups; i++) {
        bgdesc_t d = bgdt[i];
        uint16_t n_free_blocks = d.bg_free_blocks_count;
        if (n_free_blocks == 0) continue;

        *res = first_free_block(&d);
        return 0;
    }

    // Disk full
    return -1;
}

/* Find first free inode across all block groups */
int first_free_inode_num(uint32_t *res)
{
    for (int i = 0; i < n_block_groups; i++) {
        bgdesc_t d = bgdt[i];
        uint16_t n_free_inodes = d.bg_free_inodes_count;
        if (n_free_inodes == 0) continue;

        *res = first_free_inode(&d);
        return 0;
    }

    return -1;
}

/* Block bitmap operations */

void mark_block(uint32_t block_num, uint8_t val)
{
    uint16_t block_grp_n = block_num / super.s_blocks_per_group;
    bgdesc_t d = bgdt[block_grp_n];
    uint8_t bitmap[S_BLOCK_SIZE];
    disk_read_blk(d.bg_block_bitmap, bitmap);

    uint16_t bit_to_set = block_num % super.s_inodes_per_group;

    if (val == 1) {
        set_bit(bitmap, bit_to_set);
    } else {
        unset_bit(bitmap, bit_to_set);
    }

    disk_write_blk(d.bg_block_bitmap, bitmap);
}

void set_block_bitmap(uint32_t block_num)
{
    mark_block(block_num, 1);
}

void unset_block_bitmap(uint32_t block_num)
{
    mark_block(block_num, 0);
}

/* Inode bitmap operations */

void mark_inode(uint32_t inode_num, uint8_t val)
{
    uint16_t block_grp_n = (inode_num - 1) / super.s_inodes_per_group;
    bgdesc_t d = bgdt[block_grp_n];
    uint8_t bitmap[S_BLOCK_SIZE];
    disk_read_blk(d.bg_inode_bitmap, bitmap);

    uint16_t bit_to_set = (inode_num - 1) % super.s_inodes_per_group;

    if (val == 1) {
        set_bit(bitmap, bit_to_set);
    } else {
        unset_bit(bitmap, bit_to_set);
    }

    disk_write_blk(d.bg_inode_bitmap, bitmap);
}

void set_inode_bitmap(uint32_t inode_num)
{
    mark_inode(inode_num, 1);
}

void unset_inode_bitmap(uint32_t inode_num)
{
    mark_inode(inode_num, 0);
}

/* Block group descriptor operations */

void update_inode_bg_desc(uint32_t free_inode_n)
{
    uint16_t inode_bg_n = (free_inode_n - 1) / super.s_blocks_per_group;
    bgdesc_t inode_bg_desc = bgdt[inode_bg_n];
    inode_bg_desc.bg_free_inodes_count -= 1;
    inode_bg_desc.bg_used_dirs_count += 1;
}

void update_block_bg_desc(uint32_t free_block_n)
{
    /* pahile block cha group determine karaycha */
    uint16_t block_bg_n = free_block_n / super.s_blocks_per_group;
    bgdesc_t block_bg_desc = bgdt[block_bg_n];
    block_bg_desc.bg_free_blocks_count -= 1;
}

/* Sync BGDT to disk */
void disk_sync_bgdt(void)
{
    uint8_t buf[S_BLOCK_SIZE];
    uint8_t *bgdt_cast = (uint8_t *) bgdt;
    uint32_t lim = n_block_groups * sizeof(bgdesc_t);
    
    memset(buf, 0, S_BLOCK_SIZE);

    for (int i = 0; i < lim; i++) {
        *(buf + i) = *(bgdt_cast + i);
    }

    uint8_t bgdt_blockn = BGDT_BLK_NO;
    disk_write_blk(bgdt_blockn, buf);
    
    // Also write the backup block
    disk_write_blk(bgdt_blockn + super.s_blocks_per_group, buf);
}

/* Sync superblock to disk */
void disk_sync_super(void)
{
    uint8_t super_blockn = 1; 
    disk_write_blk(super_blockn, (uint8_t *) &super);
    // Write backup
    disk_write_blk(super_blockn + super.s_blocks_per_group, (uint8_t *) &super);
}

/* Reserve a free block */
int reserve_free_block(uint32_t *block_n)
{
    if (first_free_block_num(block_n)) {
        printk("No free blocks available.\n");
        return -1;
    }
    set_block_bitmap(*block_n);

    update_block_bg_desc(*block_n);
    disk_sync_bgdt();

    super.s_free_blocks_count -= 1;
    disk_sync_super();

    return 0;
}

/* Reserve an inode */
int reserve_inode(uint32_t inode_n)
{
    set_inode_bitmap(inode_n);

    update_inode_bg_desc(inode_n);
    disk_sync_bgdt();

    super.s_free_inodes_count -= 1;
    disk_sync_super();
    
    return 0;
}

/* Reserve a free inode */
int reserve_free_inode(uint32_t *inode_n)
{
    if (first_free_inode_num(inode_n)) {
        printk("No free inodes available.\n");
        return -1;
    }
    reserve_inode(*inode_n);
    return 0;
}
