#include <stdint.h>
#include "fs.h"
#include "hardware.h"
#include "disk.h"
#include "serial.h"
#include "string.h"
#include "slab.h"


// Inode table size based on the 214-block reference
// This yields 1712 inodes per group with 128-byte inodes
/* #define EXT2_INODES_PER_GROUP     ((214 * EXT2_BLK_SIZE) / sizeof(inode_t)) */

// Standard ext2 blocks per group for 1KB block size
/* #define EXT2_BLOCKS_PER_GROUP 8192 */
/*
 * aplyala basically eka block madhe bitmap basvaycha ahe.
 * Eka block chi size is 1024 bytes. 1024 * 8bits = 8192 bits.
 * Each bit for a single block. mhanun blocks per group = 8192.
 * So eka group madhe ekach block madhe 8192 blocks chi information
 * basel, basically standard ahe
 */

#define SECTORS_PER_BLOCK   (EXT2_BLK_SIZE / DISK_SECTOR_SIZE)

#define SECTORS_PER_BLOCK_GROUP (SECTORS_PER_BLOCK * EXT2_BLOCKS_PER_GROUP)

#define EXT2_BYTES_PER_INODE 8192 // this defines how much bytes each inode will cover at average
                                  // this is a standard

#define EXT2_INODES_PER_GROUP (( EXT2_BLOCKS_PER_GROUP * EXT2_BLK_SIZE ) \
        / EXT2_BYTES_PER_INODE )

/*
 * Why i've added EXT2_BLK_SIZE + 1: 
 * Reason: 
 * Assume EXT2_INODES_PER_GROUP = 10
 *          sizeof(inode_t)     = 128
 *          EXT2_BLK_SIZE = 1024
 *
 *   then inode_table_size = (EXT2_INODES_PER_GROUP * (sizeof(inode_t)) = 1280 bytes
 *   if I divide , 1280 / EXT2_BLK_SIZE it is equal to 1
 *   But As you can see 1280 > 1 block size(1024).
 *   Hence we add extra popcorn for the right calculation.
 *   Now since you add 
 *   1280 + EXT2_BLK_SIZE - 1 / EXT2_BLK_SIZE . We get 2
 *   This is right, since we need two blocks to store 1280 bytes
 */
#define EXT2_INODE_TABLE_BLOCKS ((EXT2_INODES_PER_GROUP * (sizeof(inode_t)) + EXT2_BLK_SIZE + 1) \
       / EXT2_BLK_SIZE) 

// Global filesystem parameters (dynamically calculated)
// Made non-static so they're accessible from other files
uint32_t g_n_block_groups = 0;
uint32_t g_total_blocks = 0;
uint32_t g_blocks_in_last_group = 0;

// Calculate number of block groups needed for filesystem
static uint32_t calculate_block_groups(uint32_t fs_size_mb) {
    // Convert MB to 1KB blocks: 1 MB = 1024 KB = 1024 blocks of 1KB
    g_total_blocks = fs_size_mb * 1024;
    
    // Calculate how many full block groups we need
    g_n_block_groups = (g_total_blocks + EXT2_BLOCKS_PER_GROUP - 1) / EXT2_BLOCKS_PER_GROUP;
    
    // Calculate blocks in the last group (might be partial)
    g_blocks_in_last_group = g_total_blocks % EXT2_BLOCKS_PER_GROUP;
    if (g_blocks_in_last_group == 0) {
        g_blocks_in_last_group = EXT2_BLOCKS_PER_GROUP;
    }
    
    return g_n_block_groups;
}


static uint32_t inode_table_blocks(void)
{
    return (EXT2_INODES_PER_GROUP * sizeof(inode_t)
           + EXT2_BLK_SIZE - 1) / EXT2_BLK_SIZE;
}

static uint32_t reserved_blocks_per_group(void)
{
    /* block bitmap + inode bitmap + inode table */
    return 1 + 1 + inode_table_blocks();
}

// Calculate free blocks for a specific group
static uint32_t free_blocks_in_group(uint32_t group_idx, bool has_super_backup) {
    uint32_t reserved = reserved_blocks_per_group();
    uint32_t total_blocks_in_group;
    
    // Last group might have fewer blocks
    if (group_idx == g_n_block_groups - 1) {
        total_blocks_in_group = g_blocks_in_last_group;
    } else {
        total_blocks_in_group = EXT2_BLOCKS_PER_GROUP;
    }
    
    // Subtract superblock + bgdt if this group has backups
    if (has_super_backup) {
        reserved += 2;
    }
    
    return total_blocks_in_group - reserved;
}

ext2_super_block make_super(uint16_t block_group_nr) {
    ext2_super_block b = { 0 };

    b.s_inodes_per_group = EXT2_INODES_PER_GROUP;
    b.s_blocks_per_group = EXT2_BLOCKS_PER_GROUP;
    b.s_frags_per_group = EXT2_BLOCKS_PER_GROUP;

    b.s_inodes_count = EXT2_INODES_PER_GROUP * g_n_block_groups;
    b.s_blocks_count = g_total_blocks;

    // Calculate total free blocks across all groups
    uint32_t total_free = 0;
    for (uint32_t i = 0; i < g_n_block_groups; i++) {
        // Groups 0 and 1 have superblock backups
        bool has_backup = (i < 2);
        total_free += free_blocks_in_group(i, has_backup);
    }
    b.s_free_blocks_count = total_free;

    // Reserve inodes 1 (bad blocks) and 2 (root directory)
    b.s_free_inodes_count = b.s_inodes_count - 2;

    b.s_first_data_block = 1;
    b.s_log_block_size = 0; // log2(1024) - 10 = 0 for 1KB blocks
    b.s_log_frag_size = 0;  // same as block size

    b.s_magic = EXT2_SUPER_MAGIC;
    b.s_state = EXT2_VALID_FS;
    b.s_errors = EXT2_ERRORS_RO;
    b.s_creator_os = EXT2_OS_COW;
    b.s_rev_level = 0;
    b.s_checkinterval = (1 << 24);
    b.s_first_ino = EXT2_GOOD_OLD_FIRST_INO;
    b.s_inode_size = EXT2_GOOD_OLD_INODE_SIZE; 
    b.s_block_group_nr = block_group_nr;

    return b;
}

void write_superblock(uint32_t location, ext2_super_block b) {
    disk_write(location, (uint8_t *) &b, sizeof(b));
}
/*
 * Refer to ext2_layout.md in docs 
 *
 */
void make_bgdt(bgdesc_t *table) {
    for (uint32_t i = 0; i < g_n_block_groups; i++) {
        // First 2 block groups have superblock + BGDT backups
        bool has_backup = (i < 2);
        uint8_t extra_blocks = has_backup ? 2 : 0;
        
        // Block numbers are absolute across the filesystem
        uint32_t group_base = i * EXT2_BLOCKS_PER_GROUP;
        
        table[i].bg_block_bitmap = group_base + extra_blocks + 1;
        table[i].bg_inode_bitmap = group_base + extra_blocks + 2;
        table[i].bg_inode_table = group_base + extra_blocks + 3;

        table[i].bg_free_blocks_count = free_blocks_in_group(i, has_backup);
        
        // Account for reserved inodes in first group only
        if (i == 0) {
            table[i].bg_free_inodes_count = EXT2_INODES_PER_GROUP - 2; //dummy inode and root inode
            table[i].bg_used_dirs_count = 1; // root directory sathi
        } else {
            table[i].bg_free_inodes_count = EXT2_INODES_PER_GROUP;
            table[i].bg_used_dirs_count = 0;
        }
        
        table[i].bg_pad = 0;
        memset(table[i].bg_reserved, 0, 12);
    }
}

void write_bgdt(uint32_t addr, bgdesc_t *table) {
    // Check if BGDT fits in one block
    uint32_t bgdt_size = g_n_block_groups * sizeof(bgdesc_t);
    if (bgdt_size > EXT2_BLK_SIZE) {
        printk("Error: BGDT size %u exceeds block size %u\n", 
               bgdt_size, EXT2_BLK_SIZE);
        printk("Maximum supported block groups: %u\n", 
               EXT2_BLK_SIZE / sizeof(bgdesc_t));
        sys_exit();
    }

    // Pad to full block
    uint8_t buffer[EXT2_BLK_SIZE];
    memset(buffer, 0, EXT2_BLK_SIZE);
    memcpy(buffer, table, bgdt_size);
    disk_write(addr, buffer, EXT2_BLK_SIZE);
}

static void init_bitmaps(uint32_t fs_lba,
                         uint32_t group_start,
                         uint32_t group_idx)
{
    uint8_t bitmap[EXT2_BLK_SIZE];
    memset(bitmap, 0, sizeof(bitmap));

    uint32_t reserved = reserved_blocks_per_group();
    
    // For first two groups, add superblock + BGDT
    if (group_idx < 2) {
        reserved += 2;
    }

    /* Block bitmap - mark reserved blocks as used */
    for (uint32_t i = 0; i < reserved; i++) {
        bitmap[i / 8] |= (1 << (i % 8));
    }

    disk_write(fs_lba + (group_start + 0) * SECTORS_PER_BLOCK,
               bitmap, EXT2_BLK_SIZE);

    /* Inode bitmap */
    memset(bitmap, 0, sizeof(bitmap));
    
    // Only mark inodes 1 and 2 as used in the first group
    if (group_idx == 0) {
        bitmap[0] |= 0x3;   /* inode 1 (bad), inode 2 (root) */
    }

    disk_write(fs_lba + (group_start + 1) * SECTORS_PER_BLOCK,
               bitmap, EXT2_BLK_SIZE);
}

static void init_inode_table(uint32_t fs_lba,
                             uint32_t group_start)
{
    uint8_t zero[EXT2_BLK_SIZE];
    memset(zero, 0, sizeof(zero));

    uint32_t blocks = inode_table_blocks();

    for (uint32_t i = 0; i < blocks; i++) {
        disk_write(fs_lba + (group_start + 2 + i) * SECTORS_PER_BLOCK,
                   zero, EXT2_BLK_SIZE);
    }
}

/*
 *
 * LAYOUT for 24 mb which creates 3 groups
 *  Block 0    : (unused / boot)
    Block 1    : SUPERBLOCK
    Block 2    : BGDT
    Block 3    : Block bitmap
    Block 4    : Inode bitmap
    Block 5–218: Inode table (214 blocks)
    Block 219+ : DATA BLOCKS

    Block 8192 : Dummy
    Block 8193 : SUPERBLOCK (backup)
    Block 8194 : BGDT (backup)
    Block 8195 : Block bitmap
    Block 8196 : Inode bitmap
    Block 8197–8410 : Inode table
    Block 8411+ : DATA BLOCKS

    Block 16384 : Dummy
    Block 16385 : Block bitmap
    Block 16386 : Inode bitmap
    Block 16387–16600 : Inode table
    Block 16601+ : DATA BLOCKS
 */


void mkfs(uint32_t mb_start, uint32_t len) {
    uint32_t fs_lba = fs_start_to_lba_superblk(mb_start);

    printk("========================================\n");
    printk("Creating ext2 filesystem\n");
    printk("========================================\n");
    printk("Start offset:    %u MB\n", mb_start);
    printk("Filesystem size: %u MB\n", len);
    
    // Dynamically calculate filesystem parameters
    uint32_t n_groups = calculate_block_groups(len);
    
    printk("Total blocks:    %u (1KB each)\n", g_total_blocks);
    printk("Block groups:    %u\n", n_groups);
    printk("Blocks per group: %u\n", EXT2_BLOCKS_PER_GROUP);
    printk("Inodes per group: %u\n", EXT2_INODES_PER_GROUP);
    printk("Last group size: %u blocks\n", g_blocks_in_last_group);
    printk("Inode table:     %u blocks per group\n", inode_table_blocks());
    printk("========================================\n");

    // Allocate BGDT dynamically
    bgdesc_t *bgdt = (bgdesc_t *)kmalloc(g_n_block_groups * sizeof(bgdesc_t), 0);
    if (!bgdt) {
        printk("Error: Failed to allocate memory for BGDT\n");
        sys_exit();
    }

    // Create block group descriptor table
    make_bgdt(bgdt);

    // Create and write superblocks
    for (uint32_t i = 0; i < g_n_block_groups; i++) {
        // Only write superblock to groups 0 and 1 (backups)
        if (i < 2) {
            ext2_super_block sb = make_super(i);
            uint32_t sb_addr = fs_lba + (i * SECTORS_PER_BLOCK_GROUP);
            write_superblock(sb_addr, sb);
            printk("Group %u: wrote superblock at LBA %u\n", i, sb_addr);
        }
    }


    // Write BGDT after superblock in groups 0 and 1
    write_bgdt(fs_lba + SECTORS_PER_BLOCK, bgdt);
    write_bgdt(fs_lba + SECTORS_PER_BLOCK_GROUP + SECTORS_PER_BLOCK, bgdt);
    printk("Wrote block group descriptor tables\n");

    // Initialize each block group
    for (uint32_t i = 0; i < g_n_block_groups; i++) {
        // Calculate where bitmaps start (skip superblock + bgdt in first 2 groups)
        uint32_t bitmap_start = (i < 2) ? 2 : 0;
        uint32_t block_offset = i * EXT2_BLOCKS_PER_GROUP + bitmap_start;
        
        printk("Group %u: initializing bitmaps and inode table\n", i);
        init_bitmaps(fs_lba, block_offset, i);
        init_inode_table(fs_lba, block_offset);
    }

    // Free allocated BGDT
    kfree(bgdt);

    printk("Filesystem structure created, finalizing...\n");
    finish_fs_init(mb_start);
    //TODO:
    /*
     * create the root_directory 
     */
    printk("========================================\n");
    printk("mkfs complete!\n");
    printk("========================================\n");
}

// Helper function to display filesystem info
// This is separate from read_fs() which is in fs.c
void show_fs(uint32_t mb_start) {
    uint32_t fs_lba = fs_start_to_lba_superblk(mb_start);
    
    // Read the superblock
    ext2_super_block sb;
    uint8_t sb_buffer[EXT2_BLK_SIZE];
    disk_read(fs_lba, sb_buffer);
    memcpy(&sb, sb_buffer, sizeof(ext2_super_block));

    printk("fs_lba for superblock is %u\n", fs_lba);
    
    // Validate magic number
    if (sb.s_magic != EXT2_SUPER_MAGIC) {
        printk("Error: Invalid ext2 magic number: 0x%x (expected 0x%x)\n", 
               sb.s_magic, EXT2_SUPER_MAGIC);
        return;
    }
    
    printk("========================================\n");
    printk("Filesystem Information\n");
    printk("========================================\n");
    printk("Magic:           0x%x\n", sb.s_magic);
    printk("State:           %u (%s)\n", sb.s_state, 
           sb.s_state == EXT2_VALID_FS ? "VALID" : "ERROR");
    printk("Total blocks:    %u\n", sb.s_blocks_count);
    printk("Free blocks:     %u\n", sb.s_free_blocks_count);
    printk("Total inodes:    %u\n", sb.s_inodes_count);
    printk("Free inodes:     %u\n", sb.s_free_inodes_count);
    printk("Block size:      %u bytes\n", 1024 << sb.s_log_block_size);
    printk("Blocks/group:    %u\n", sb.s_blocks_per_group);
    printk("Inodes/group:    %u\n", sb.s_inodes_per_group);
    printk("First inode:     %u\n", sb.s_first_ino);
    printk("Inode size:      %u bytes\n", sb.s_inode_size);
    
    // Calculate and display block groups
    uint32_t n_groups = (sb.s_blocks_count + sb.s_blocks_per_group - 1) 
                        / sb.s_blocks_per_group;
    printk("Block groups:    %u\n", n_groups);
    printk("========================================\n");
     // Update global state for filesystem operations
    g_n_block_groups = n_groups;
    g_total_blocks = sb.s_blocks_count;
    g_blocks_in_last_group = sb.s_blocks_count % sb.s_blocks_per_group;
    if (g_blocks_in_last_group == 0) {
        g_blocks_in_last_group = sb.s_blocks_per_group;
    }
   
}
