#ifndef _UFS_H
#define _UFS_H

#include <stdint.h>
#include "hardware.h"
#include "disk.h"
#include "slab.h"
#include "mm.h"

#define MAX_BITMAP_BLOCKS 10

struct ufs_super_block {
    uint32_t s_fs_size;
    uint32_t s_free_blocks_count;
    uint32_t s_blk_bitmap[MAX_BITMAP_BLOCKS]; //this contains the block numbers containing bitmaps of block free/not
    uint8_t s_blk_bitmap_blks; //total required blocks out of 10
  
    uint32_t s_inode_bitmap[MAX_BITMAP_BLOCKS]; //this contains the block numbers containing bitmaps of block free/not
    uint8_t s_inode_bitmap_blks; //total required blocks out of 10

    uint32_t *s_


};

#endif
