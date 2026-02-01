#include "ufs.h"
#include "hardware.h"
#include "disk.h"
#include "slab.h"
#include "mm.h"
#include "serial.h"
#include "string.h"
#include "list.h"
#include "kernel.h"
#include "buffer.h"
#include "task.h"
#include <stdint.h>

#define SECTORS_PER_BLOCK   (U_BLK_SIZE / DISK_SECTOR_SIZE)
#define U_BYTES_PER_INODE 4096
#define SUPERBLOCK_OFFSET   SECTORS_PER_BLOCK
#define INODES_PER_BLOCK (U_BLK_SIZE / sizeof(struct d_inode))

#define INODES 50 //total no of inodes present in hashqueues 

extern struct task_struct *current;
uint32_t filesys_start = 0;
uint8_t fs_start_set = 1;
struct incore_table itable;
struct list_head file_table;

s_ufs *super = NULL;

kmem_cache_t *inode_cache;
kmem_cache_t *file_cache; //for file table entries


int create_root(void);
void display_itable(void);

struct inode_cache {
    struct list_head i_hash[DEV_NO];
    struct list_head i_free;
}i_cache;

void display_sb(s_ufs *sb);

int disk_read_blk(uint32_t block_num, uint8_t *buf) {
    if (!fs_start_set) return 1;

    uint32_t lba = filesys_start + block_num * SECTORS_PER_BLOCK;

    for (int i = 0; i < SECTORS_PER_BLOCK; i++) {
        disk_read(lba + i, buf + DISK_SECTOR_SIZE * i);
    }
    return 0;
}

int disk_write_blk(uint32_t block_num, uint8_t *buf) {
    if (!fs_start_set) return -1;

    uint32_t lba = filesys_start + block_num * SECTORS_PER_BLOCK;
    for (int i = 0; i < SECTORS_PER_BLOCK; i++) {
        disk_write(lba + i, buf + DISK_SECTOR_SIZE * i, DISK_SECTOR_SIZE);
    }
    return 0;
}

int bmap(struct inode *inode, unsigned long byte_offset)
{
    return byte_offset / U_BLK_SIZE;
}

/*
 * directory entry layout is 
 * 4 byte inode + 16 byte name 
 */
uint32_t search_dir(struct inode *inode, const char *entry, int entry_len) 
{
    unsigned long offset = 0;
    unsigned long len = inode->i_size;
    int block = (inode->i_size / U_BLK_SIZE ) + 1;
    int iCnt = 0;
    struct buffer_head *bh = NULL;
    uint32_t inum;

    while (iCnt < block) {
        bh = bread(DEV_NO, inode->i_blocks[iCnt]);    
        if (!bh) {
            printk("failed bread in search_dir\n");
        }
        offset += DIR_ENTRY_INODE_SIZE;
        while (offset < U_BLK_SIZE) {
           if (strncmp(bh->b_data + offset, entry, entry_len) == 0) {
               inum = *(uint32_t *)(bh->b_data + offset - DIR_ENTRY_INODE_SIZE); 
               goto conclude;
           } 
           offset += DIR_ENTRY_BYTES;
        }
        brelse(bh);
        iCnt++;
    }
    return -1;

conclude:
    brelse(bh);
    return inum;
}

/*
     @brief: returns index of the component where string end or next char=/ 
*/
static inline int next_token(const char* str,int start,int len){
     int end=start;
     while(end < len && str[end]!= '/'  ){    
          end++;
     }
     return end-1;
}

struct inode *namei(const char *path)
{
    struct inode *work = NULL;
    const char *look = path;
    int index = 0, len = 0;
    uint32_t inode = 0;

    if ((len = strlen(path)) <= 0) {
        return NULL;
    }

    if (path[0] == '/') {
        /* work = iget(DEV_NO, 1); //get the root directory */
        work = current->root;
        index++;
    }
    else {
        work = current->pwd;
    }

    while (index < len) {
        char *curr_token = NULL;
        int end;
        if (look[index] == '/') {
            end = next_token(path, index, len);
            curr_token = (char*)kmalloc(sizeof(char)*(end-index+1), 0);
            if (!curr_token) {
                printk("failed kmalloc for curr_token\n");
            }
            strncpy(curr_token, look+index, end-index+1);
            inode = search_dir(work, curr_token, end-index+1);
            if (inode == -1) {
                printk("inode not found in directory \n");
                kfree(curr_token);
                return NULL;
            }
            
        }
        kfree(curr_token);
        iput(work);
        work = iget(DEV_NO, inode);
        index = end + 1;
    }

    return work;
}

void fill_free_inode_list(s_ufs *super)
{
    bool bFlag = false;
    struct buffer_head *bh;
    struct d_inode *dinode;

    unsigned long dilb_start = super->s_blk_dilb_start + 
        (super->s_remembered_inode / INODES_PER_BLOCK);
    unsigned long dilb_end = super->s_blk_dilb_end;

    int dCnt = 0, list_fill_size = super->s_n_free_inodes >= FREE_INODE_LIST 
               ? FREE_INODE_LIST: super->s_n_free_inodes;

    int i_free_cnt = 0;

    while (dilb_start <= dilb_end) {

        bh = bread(DEV_NO, dilb_start);

        if (!bh) {
            printk("failed bread for dilb start %d\n", dilb_start);
            return;
        }
        dinode = (struct d_inode*)bh->b_data;
        dCnt = U_BLK_SIZE / sizeof(struct d_inode);

        while (dCnt--) {
            if (dinode->i_mode == 0) {
                super->s_free_inodes[i_free_cnt++] = dinode->i_no;
                super->s_next_free_inode++;
            }
            if (i_free_cnt == FREE_INODE_LIST) {
                bFlag = true;
                super->s_remembered_inode = dinode->i_no + 1;
                break;
            }
            dinode++;
        }
        dilb_start++;
        brelse(bh);
        if (bFlag) {
            break;
        }
        
    }
}

void init_dinode(struct d_inode *dinode, int dev_no, unsigned long i_no, uint32_t blkno)
{
    int iCnt = 0;
    dinode->i_mode = 0;
    dinode->i_uid = 0;
    dinode->i_size = 0;
    dinode->i_atime = 0;
    dinode->i_ctime = 0;
    dinode->i_mtime = 0;
    dinode->i_dtime = 0;
    dinode->i_gid = 0;
    dinode->i_nlinks = 0;
    dinode->i_n_blocks = 0;
    dinode->i_flags = 0;
    dinode->i_dev = dev_no;
    for (iCnt = 0; iCnt < INODE_MEMBER_BLOCKS; iCnt++) {
        dinode->i_blocks[iCnt] = 0;
    }
    dinode->i_no = i_no;
    dinode->i_blkno = blkno;
    dinode->i_dir_acl = 0;
    dinode->i_faddr = 0;
}


void init_inode(struct inode *inode, int dev_no, unsigned long i_no, uint32_t blkno)
{
    init_dinode((struct d_inode*)inode, dev_no, i_no, blkno);
    inode->i_state = 0;
}

/*
 * algorithm ialloc:
 * assigns a new disk inode to a newly created file
 * An inode is free, if its filetype is 0.
 * To determine the free inodes kernel would have to go through DILB, 
 * which means expensive disk operations search each one linearly. 
 * To avoid this SuperBlock maintains a list of free inodes. 
 * When the list is emptied, its updated immediately on the next allocation
 * operation.
 */
struct inode *ialloc(short dev_no)
{
    int i_no = 0;
    struct inode *inode = NULL;
    while (1) {
        if (LOCKED(super->s_inode_list_lock)) {
            //sleep until lock is released
            continue;
        }

        if (super->s_next_free_inode == -1) {
            printk("free inode list in super block is empty \n");
            LOCK(&super->s_inode_list_lock);
            fill_free_inode_list(super);
        }
        /* free inodes available */
        
        i_no = super->s_free_inodes[super->s_next_free_inode--];
        inode = iget(DEV_NO, i_no);
        if (!inode) {
            printk("failed to get inode %d\n", i_no);
            return NULL;
        }
        printk("got inode number %d\n", i_no);
        /* if inode not free after all !!! */
        /* { */
        /*    write inode to disk; */
        /*   release inode;(algorithm iput); */
        /*     continue; */
        /* } */ 

        /* inode is free */
        /* printk("inode is free\n"); */
        uint32_t i_blk = super->s_blk_dilb_start + 
             (i_no / INODES_PER_BLOCK);
        init_inode(inode, dev_no, i_no, i_blk);
        super->s_n_free_inodes--;
        /* printk("inode ok\n"); */

        return locked_inode(inode);
        
    }
}

void ifree(short dev_no, unsigned long i_no)
{
    super->s_n_free_inodes++;
    if (LOCKED(super->s_inode_list_lock)) {
        printk("super block is locked \n");
        return;
    }
    if (super->s_next_free_inode == FREE_INODE_LIST) {
        if (i_no < super->s_remembered_inode) {
            super->s_remembered_inode = i_no;
        }
    }
    else {
        super->s_free_inodes[super->s_next_free_inode++] = i_no;
    }
    return;
}

void make_block_list(unsigned long fs_bytes)
{
    
    unsigned long start_blk = super->s_blk_dilb_end + 1;
    unsigned long end_blk = (fs_bytes / (unsigned long)U_BLK_SIZE) - 1;

    int bCnt = 0;

    unsigned long iblk = start_blk;
    unsigned long iCnt = iblk;
    unsigned long link_blk = 0;
    bCnt = FREE_BLOCK_LIST - 1;

    /* printk("  i_blk = %lu, bCnt = %d\n", iblk, bCnt); */
    /* Fill the initial superblock free block array */
    for (; bCnt >= 0 && iblk <= end_blk; bCnt--, iblk++ ){
       super->s_free_blocks[bCnt] = iblk;
    }
    /* printk("s_free_blocks[%d] = %lu\n", FREE_BLOCK_LIST-1, super->s_free_blocks[FREE_BLOCK_LIST-1]); */
    /* printk("  i_blk = %lu, bCnt = %d\n", iblk, bCnt); */

    /* If we've exhausted all blocks, we're done */
    if (iblk > end_blk) {
        printk("All blocks fit in superblock array\n");
        return;
    }

    link_blk = super->s_free_blocks[0];
    /* printk("first link block is %lu\n", link_blk); */
    struct buffer_head *bh;

    while (iblk <= end_blk) {

        bh = bread(DEV_NO, link_blk);
        if (!bh) {
            printk("failed bread blkno %lu\n", link_blk);
            return;
        }
        /* printk("[ link_blk = %lu, iblk = %lu ]\n", link_blk, iblk); */
        
        // Zero out the buffer to avoid garbage 
        memset(bh->b_data, 0, U_BLK_SIZE);

        bCnt = FREE_BLOCK_LIST - 1;
        iCnt = iblk;

        /* Fill this link block with free block numbers.
         * Index 0 will contain the pointer to the next link block.
         * Indices 1-127contain actual free block numbers.
         */
        for (; iCnt <= end_blk && bCnt >= 0; bCnt--, iCnt++){
            ((unsigned long*)bh->b_data)[bCnt] = iCnt;         
        }
        
       
        link_blk = ((unsigned long*)bh->b_data)[0];

        bwrite(bh);
        
        /* Move to the next batch of blocks */
        iblk = iCnt;
        
        /* If the next link block is 0, we're done */
        if (link_blk == 0) {
            break;
        }
    }
}

void make_super(int mb) 
{
    int iCnt = 0;
    unsigned long fs_bytes;
    unsigned long inode_blocks;
    struct buffer_head *bh;

    super->s_fs_size = mb; //in mb 

    fs_bytes = (unsigned long)mb * 1024 * 1024;

    /* inode area */
    //inode count total
    super->s_total_inodes = fs_bytes / U_BYTES_PER_INODE;

    //inode table(dilb) calculation
    inode_blocks =
        (super->s_total_inodes + INODES_PER_BLOCK - 1) /
        INODES_PER_BLOCK;

    super->s_blk_dilb_start = 2;
    super->s_blk_dilb_end   = super->s_blk_dilb_start + inode_blocks - 1;

    super->s_inode_list_size = FREE_INODE_LIST;

    super->s_n_free_inodes = super->s_total_inodes;
    super->s_next_free_inode = FREE_INODE_LIST - 1;
    /* Initialize free block index
     * skip inode 0 and 1(they're not free inodes), hence its iCnt + 2
     * inode 0 is dummy inode
     * 
     */
    for (iCnt = 0; iCnt < FREE_INODE_LIST; iCnt++) {
        super->s_free_inodes[iCnt] = iCnt + 2;
    }
    super->s_remembered_inode = iCnt + 2;
    super->s_inode_list_lock = 0;
    super->s_flags = 0;

    /*  block area */
    /* remember that data blocks start from next block of s_blk_dilb_end (end of dilb) */
    
    super->s_next_free_block = FREE_BLOCK_LIST - 1;
    unsigned long start_blk = super->s_blk_dilb_end + 1;
    unsigned long end_blk = (fs_bytes / (unsigned long)U_BLK_SIZE) - 1;
    super->s_n_free_blocks = end_blk - start_blk + 1; //Initialize free blocks count

    printk("Before make_block_list: s_next_free_block = %d\n", super->s_next_free_block);
    make_block_list(fs_bytes);
    printk("After make_block_list: s_next_free_block = %d\n", super->s_next_free_block);
}

void mkufs(int mb)
{
    int iCnt = 0, jCnt = 0;
    struct buffer_head *bh;
    bh = bread(DEV_NO, 1);
    if (!bh) {
        printk("read super blocked failed \n");
        return;
    }
    super = (s_ufs*)bh->b_data;
    printk("making super\n");
    /* block part is made in make_super */
    make_super(mb);

    /* inode part */

    for (iCnt = 0; iCnt < super->s_total_inodes; iCnt += INODES_PER_BLOCK) {

        uint32_t inodes_block = (iCnt / INODES_PER_BLOCK) 
            + super->s_blk_dilb_start; 
        /* printk("iCnt: %d, its_block: %d\n", iCnt, inodes_block); */
        struct buffer_head *binode = NULL;
        binode = bread(DEV_NO, inodes_block);
        if (!binode) {
            printk("bread failed at %d\n", inodes_block);
            break;
        }
        for (jCnt = 0; jCnt < INODES_PER_BLOCK; jCnt++) {
            struct d_inode *dinode = (struct d_inode*)binode->b_data + jCnt;
            init_dinode(dinode, DEV_NO, iCnt + jCnt, inodes_block);
        }
        /* printk("iCnt: %d, jCnt: %d\n", iCnt, jCnt); */

        bwrite(binode);
    }

    iCnt = create_root();
    if (iCnt == -1) {
        printk("creation of root failed\n");
        return;
    }

    bwrite(bh);
    

}

struct d_inode get_dinode(unsigned long i_no)
{
    struct d_inode dinode;
   
    struct buffer_head *bh;

    uint32_t i_blk = (i_no / INODES_PER_BLOCK) + super->s_blk_dilb_start; 

    uint16_t i_blk_offset = (i_no % INODES_PER_BLOCK);

    bh = bread(DEV_NO, i_blk);
    if (!bh) {
        printk("bread failed for %u\n", i_blk);
    }

    dinode = *((struct d_inode*)bh->b_data + i_blk_offset);

    brelse(bh);

    return dinode;
}

void read_inode(struct inode *inode)
{
    struct d_inode dinode;
    dinode = get_dinode(inode->i_no);
    
    *(struct d_inode*)inode = dinode;

}

void write_inode(struct inode *inode)
{
   
    struct buffer_head *bh;

    uint32_t i_blk = (inode->i_no / INODES_PER_BLOCK) + super->s_blk_dilb_start; 

    uint16_t i_blk_offset = (inode->i_no % INODES_PER_BLOCK);

    bh = bread(DEV_NO, i_blk);
    if (!bh) {
        printk("bread failed for %u\n", i_blk);
    }

    *((struct d_inode*)bh->b_data + i_blk_offset) = *(struct d_inode*)inode;

    bwrite(bh);
}

void display_inode_cache(void) 
{
    int iCnt = 0;
    struct list_head *run;
    struct inode *inode;

    for (; iCnt < DEV_NO; iCnt++) {
             printk("================ i_hash %d=================\n", iCnt);
        list_for_each(run, &i_cache.i_hash[iCnt]) {
            inode = list_entry(run, struct inode, i_hash);
            printk("<= %d => ", inode->i_no);
        }
        printk("\n");
    }
    printk("\n=================== i_free ========================\n");
    list_for_each(run, &i_cache.i_free) {
        inode = list_entry(run, struct inode, i_free);
        printk("<= %d => ", inode->i_no);
    }
    printk("\n=================== i_free ========================\n");
    printk("\n");

}
static struct inode *search_inode_hash(unsigned short dev_no, unsigned long inum)
{
    struct inode *inode = NULL;
    struct list_head *run;
    int index = hash_fn((int)inum, dev_no);

    list_for_each(run, &i_cache.i_hash[index]) {
        inode = list_entry(run, struct inode, i_hash);
        if (inode->i_no == inum && inode->i_dev == dev_no) {
            return inode;
        }
    }
    printk("search_inode hash failed\n");
    return NULL;
}


void inode_init(struct inode *inode, unsigned short dev_no, unsigned long inum)
{

    inode->i_no = inum;
    read_inode(inode);
    INIT_LIST_NULL(&inode->i_hash);
    INIT_LIST_NULL(&inode->i_free);
    inode->i_state = 0;
    inode->i_count = 0;
}

void test_icache(void)
{
    printk("testing inode cache .............\n");
    struct inode *inode = iget(DEV_NO, 16);
    if (!inode) {
        printk(" iget failed \n");
        return ;
    }
    printk("iget passed inode->i_no : %d\n", inode->i_no);
   
}
void init_file(void *fobj, kmem_cache_t *cachep, unsigned long no)
{
    struct file *file = (struct file*)fobj;
    file->f_inode = NULL;
    INIT_LIST_NULL(&file->f_list);
    file->f_count = 0;
    file->f_flags = 0;
    file->f_mode = 0;
    file->f_pos = 0;
}

void create_file_cache(void)
{
    file_cache = kmem_cache_create("file", sizeof(struct file), KMALLOC_MINALIGN, SLAB_HWCACHE_ALIGN, init_file);
    if (!file_cache) {
        printk("failed kmem_cache_create for file_cache \n");
        return;
    }
}

void create_inode_cache(void)
{

    int iCnt = 0;
    struct inode *tmp = NULL;

    inode_cache = kmem_cache_create("inode", sizeof(struct inode), KMALLOC_MINALIGN, SLAB_HWCACHE_ALIGN, NULL);
    if (!inode_cache) {
        printk("failed kmem_cache_create for inode\n");
        return;
    }

    create_file_cache();

    for (iCnt = 0; iCnt < DEV_NO; iCnt++) {
        INIT_LIST_HEAD(&i_cache.i_hash[iCnt]);
    }
    INIT_LIST_HEAD(&i_cache.i_free);
    /*
     * inodes must start from 1 and not 0
     */

    for (iCnt = 1; iCnt <= INODES; iCnt++) {
        tmp = kmem_cache_alloc(inode_cache, 0); 
        if (!tmp) {
            printk("failed allocating inode for %d\n", iCnt);
            break;
        }
       
        inode_init(tmp, DEV_NO, iCnt);
        /*
         * add to correct hash list and also to freelist */
        list_add(&i_cache.i_hash[hash_fn(iCnt, DEV_NO)], &tmp->i_hash);
        list_add(&i_cache.i_free, &tmp->i_free);
    }

    printk("size of inode is %d\n", sizeof(struct inode));
    printk("displaying inode cache................\n");
    display_inode_cache();
    test_icache();
}

struct inode *iget(unsigned short dev_no, unsigned int inum)
{
    printk("getting inode no: %d\n", inum);
    struct inode *inode = NULL;
    while (1) {
        inode = search_inode_hash(dev_no, inum); 
        if (inode) {
            if (IS_FLAG(inode->i_flags, I_LOCKED)) {
                /*
                 * sleep (event this inode becomes free )
                 */
                printk("inode locked\n");
                continue;
            }
            if (inode->i_count == 0) { //inode is on free list
                printk("inode is on free list\n");
                list_del(&inode->i_free);  
            }
            printk("returning inode\n");
            inode->i_count++;
            return locked_inode(inode);
        }
        if (list_is_empty(&i_cache.i_free)) {
            printk("inode free list is empty\n");
            return NULL;
        }

        inode = list_first_entry(&i_cache.i_free, struct inode, i_free);

        list_del(&inode->i_free);
        inode->i_no = inum;
        /* ino->dev_no = dev_no; */
        list_del(&inode->i_hash);
        list_add(&i_cache.i_hash[hash_fn(inode->i_no, dev_no)], &inode->i_hash); 

        //read inode from disk via bread at core 
        /* ext2_read_inode(inode); */ 
        read_inode(inode);

        inode->i_count++; 

        return locked_inode(inode);
    }
    return NULL;
}

void iput(struct inode *inode)
{
    inode = locked_inode(inode);
    inode->i_count--;

    if (inode->i_count == 0) {
        /* if (inode->i_nlinks == 0) { */
        /*     //free disk blocks for file ( bach ch4 Disk Blocks ) */
        /*     //set file type to 0 */
        /*     //free inode (ifree) */
        /* } */
        if (IS_FLAG(inode->i_state, I_DIRTY_INODE) || IS_FLAG(inode->i_state, I_DIRTY_INODE)) {
            printk("inode %u is dirty, writing inode to disk\n", inode->i_no);
           write_inode(inode); //update the disk inode 
        }
    }
    list_add_tail(&i_cache.i_free, &inode->i_free);

    unlocked_inode(inode);
}


void display_sb(s_ufs *sb)
{
    int i;

    printk("========== UFS SUPER BLOCK ==========\n");

    printk("Filesystem size        : %d\n", sb->s_fs_size);

    printk("Inode list size        : %u\n", sb->s_inode_list_size);
    printk("Total inodes           : %lu\n", sb->s_total_inodes);
    printk("Free inodes count      : %lu\n", sb->s_n_free_inodes);

    printk("Next free inode index  : %d\n", sb->s_next_free_inode);
    printk("Remembered inode       : %lu\n", sb->s_remembered_inode);

    printk("Inode list lock        : %d\n", sb->s_inode_list_lock);
    printk("Superblock flags       : 0x%x\n", sb->s_flags);

    printk("DILB start block       : %d\n", sb->s_blk_dilb_start);
    printk("DILB end block         : %d\n", sb->s_blk_dilb_end);

    printk("Free inode list        : ");
    for (i = 0; i < FREE_INODE_LIST; i++) {
        printk("%u ", sb->s_free_inodes[i]);
    }
    printk("\n");

    printk("Free blocks: %u\n", sb->s_n_free_blocks);
    printk("Next free block: %d\n", sb->s_next_free_block);

    printk("Free block list        : ");
    for (i = 0; i < FREE_BLOCK_LIST; i++) {
        printk("%lu ", sb->s_free_blocks[i]);
    }
    printk("\n");


    printk("=====================================\n");
}

struct buffer_head *balloc(unsigned short dev_no)
{
    unsigned long blk_no;
    struct buffer_head *bh;
    while (LOCKED(super->s_block_list_lock)) {
        continue;
    }

    if (super->s_next_free_block < 0) {
        printk("balloc: block list underflow\n");
        return NULL;
    }

    blk_no = super->s_free_blocks[super->s_next_free_block--];

    if (super->s_next_free_block == -1) {
        LOCK(&super->s_block_list_lock);
        bh = bread(DEV_NO, blk_no);
        if (!bh) {
            printk("failed bread for blkno %lu\n", blk_no);
            // Revert state so we don't stay in -1?
            // But we lost the link block. FS is effectively stuck/corrupted or read error.
            // Leaving it at -1 prevents further damage via underflow check above.
            return NULL;
        }
        printk("super free block is empty , now filling data from blkno: %lu\n", blk_no);
        unsigned long *ptr = (unsigned long*)bh->b_data;
        int blocks = FREE_BLOCK_LIST;
        int iCnt = 0;
        for (iCnt = 0; iCnt < blocks; iCnt++) {
            super->s_free_blocks[iCnt] = ptr[iCnt];
        }
        super->s_next_free_block = blocks - 1;
        brelse(bh);
        UNLOCK(&super->s_block_list_lock);
        //wakeup all processes waiting for buffer
    }
    bh = getblk(DEV_NO, blk_no);
    if (!bh) {
        printk("getblk failed for blkno %lu\n", blk_no);
        return NULL;
    }
    /* memset(bh->b_data, 0, U_BLK_SIZE); */
    super->s_n_free_blocks--;
    SET_FLAG(super->s_flags, SUPER_MODIFIED);
    return bh;
}

void bfree(unsigned long blk_no)
{
    while (LOCKED(super->s_block_list_lock)) {
        printk("super block free block list is locked \n");
        continue;
    }

    LOCK(&super->s_block_list_lock);

    if (super->s_next_free_block >= FREE_BLOCK_LIST) {
        struct buffer_head *bh;
        bh = bread(DEV_NO, blk_no);
        if (!bh) {
            printk("failed bread for blkno %lu\n", blk_no);
            return;
        }
        unsigned long *ptr = (unsigned long*)bh->b_data;
        int blocks = FREE_BLOCK_LIST;
        int iCnt = 0;
        for (iCnt = 0; iCnt < blocks; iCnt++) {
            ptr[iCnt] = super->s_free_blocks[iCnt];
        }
        super->s_free_blocks[0] = blk_no;
        super->s_next_free_block = 0;
        bwrite(bh);
    } 
    else {
        super->s_free_blocks[super->s_next_free_block++] = blk_no;
    }

    UNLOCK(&super->s_block_list_lock);
}



void test_fs(void)
{
    /* Update global super pointer to point to valid buffer data */

    if (!super) {
        struct buffer_head *bh;
        bh = bread(DEV_NO, 1);
        if (!bh) {
            printk("failed bread for block 1\n");
            return;
        }
        super = (s_ufs*)bh->b_data;
    }
    display_sb(super);

    /* struct buffer_head *btest[2]; */
    /* printk("tesint dilb==========================\n"); */
    /* for (int iCnt = 0; iCnt < 2; iCnt++) { */
    /*     btest[iCnt] = bread(DEV_NO, (unsigned long)(sb->s_blk_dilb_start + iCnt)); */
    /*     if (!btest[iCnt]) { */
    /*         printk("bread failed at blkno %lu\n", sb->s_blk_dilb_start + iCnt); */
    /*     } */
    /*     struct d_inode *dinode = (struct d_inode*)btest[iCnt]->b_data; */
    /*     for (int jCnt = 0; jCnt < U_BLK_SIZE / sizeof(struct d_inode); jCnt++) { */
    /*         printk("jCnt : %d, inode: %u, imode: %d\n", jCnt, dinode[jCnt].i_no, (int)dinode[jCnt].i_mode); */
    /*     } */
    /*     brelse(btest[iCnt]); */
    /* } */
    /* printk("done tesint dilb==========================\n"); */

    /* printk("testing data blocks==================\n"); */

    /* int iCnt = 0; */

    /* int link_blk = 513; */

    /* for (iCnt = 0; iCnt < 2; iCnt++) { */

    /*     btest[iCnt] = bread(DEV_NO, (unsigned long)(link_blk + (iCnt * FREE_BLOCK_LIST))); */
    /*     if (!btest[iCnt]) { */
    /*         printk("bread failed at blkno %lu\n", link_blk  + (iCnt * FREE_BLOCK_LIST)); */
    /*     } */

    /*     printk("readin link block no %lu\n", (unsigned long)(link_blk + (iCnt *FREE_BLOCK_LIST))); */
    /*     unsigned long *ptr = (unsigned long*)btest[iCnt]->b_data; */
    /*     for (int jCnt = 0; jCnt < U_BLK_SIZE / sizeof(unsigned long); jCnt++) { */
    /*         printk("jCnt : %d, block: %lu\n", jCnt, ptr[jCnt]); */
    /*     } */
    /*     brelse(btest[iCnt]); */
    /* } */
    /* printk("done testing data blocks==================\n"); */

    /* printk("testing alloc and free===========\n"); */

    /* int iCnt = 0; */
    /* struct buffer_head *bh[98]; */
    /* for (iCnt = 0; iCnt < 258; iCnt++) { */
    /*    bh[iCnt % 98] = balloc(DEV_NO); */
    /*     if (!bh[iCnt % 98]) { */
    /*        printk("balloc failed at iCnt: %d\n", iCnt); */
    /*       return; */
    /*     } */ 
    /*   printk("balloc got me %lu\n", bh[iCnt % 98]->b_blocknr); */
    /*   if ((iCnt + 1) % 98 == 0) */
    /*   { */
    /*         int jCnt = 0; */
    /*         for (jCnt = 0; jCnt < 98; jCnt++) { */
    /*             bwrite(bh[jCnt]); */
    /*             //bfree(bh[jCnt]->b_blocknr); */
    /*         } */
    /*   } */
    /*   if (iCnt % 256 == 0) { */
    /*       printk("round %d  finished================\n", iCnt % 256); */
    /*   } */
    /* } */
    
    /* // flush remaining */
    /* int remaining = iCnt % 98; */
    /* for (int jCnt = 0; jCnt < remaining; jCnt++) { */
    /*     bwrite(bh[jCnt]); */
    /*     /1* bfree(bh[jCnt]->b_blocknr); *1/ */
    /* } */
    /* iCnt = 770; */
    /* for (; iCnt>= 513; iCnt--){ */
    /*     bfree((unsigned long)iCnt); */
    /* } */
   
    display_itable();
}

int alloc_incore_entry(struct inode *inode)
{
    int iCnt = 0;
    for (iCnt = 0; iCnt < INCORE_TABLE; iCnt++) {
        if (itable.table[iCnt] == NULL) {
            itable.table[iCnt] = inode;
            return iCnt;
        }
    }
    return -1;
 
}

/*
 * allocates file table and ufdt entry as well
 * @paramter: incore inode table entry's index
 */
int alloc_file_entry(int incore_entry)
{
    int fd;
    struct file *file = kmem_cache_alloc(file_cache, 0);
    if (!file) {
        printk("file object allocation failed\n");
        return -1;
    }
    
    fd = current->first_free_filp;
    current->filp[current->first_free_filp++] = file; //allocated ufdt entry 
    file->f_inode = itable.table[incore_entry]; //allocate file table entry
    list_add_tail(&file_table, &file->f_list);

    return fd;

}

int allocate_file(struct inode *inode)
{
    int fd = -1;
    int iCnt = 0;
    if (current->first_free_filp == NR_OPEN) {
        printk("maximum files opened, cannot open more\n");
        return -1;
    }
    //allocate inode entry in inode table
    for (iCnt = 0; iCnt < INCORE_TABLE; iCnt++) {
        if (itable.table[iCnt] == NULL) {
            itable.table[iCnt] = inode;
        }
    }
    
    struct file *file = kmem_cache_alloc(file_cache, 0);
    if (!file) {
        printk("file object allocation failed\n");
        return -1;
    }
    
    fd = current->first_free_filp;
    current->filp[current->first_free_filp++] = file; //allocated ufdt entry 
    file->f_inode = inode; //allocate file table entry
    list_add_tail(&file_table, &file->f_list);

    return fd;
}

void create_entry(const char *entry_name, struct buffer_head *bh,
        unsigned long offset, unsigned long inode_no)
{
    struct dir_entry dentry;
    dentry.dir_inode = inode_no;
    strcpy(dentry.dir_name, entry_name);

    /* char *ptr = (char*)bh->b_data; */
    /* *(unsigned long*)(ptr + offset) = inode_no; */
    /* strcpy(ptr + offset + DIR_ENTRY_INODE_SIZE, entry_name); */

    memcpy(bh->b_data + offset, &dentry, sizeof(struct dir_entry));
}

void display_itable(void)
{
    printk("displaying incore inode table..........\n");

    /* struct inode *inode = itable.table[0]; */
    /* int iCnt = 0; */
    /* while (inode != NULL) { */
    /*     printk("index %d, inode->i_no: %u\n", iCnt, inode->i_no); */
    /*     inode++; */
    /*     iCnt++; */
    /* } */

}

int mount_rootfs(void)
{
    printk("mounting rootfs............\n");

    loff_t iCnt = 0;
    struct buffer_head *bh = NULL;
    struct dir_entry *dentry = NULL;
    struct inode *inode = NULL;

    inode = iget(DEV_NO, 1);
    if (!inode) {
        printk("failed to get inode 1\n");
        return -1;
    }
    read_inode(inode);

    loff_t file_size = inode->i_size;
    bh = bread(DEV_NO, inode->i_blocks[0]);
    if (!bh) {
        printk("failed to get buffer\n");
        return -1;
    }
    alloc_incore_entry(inode);
    current->root = current->pwd = inode;

    printk("root's file size is %lu\n", file_size);

    /* just print the root directory */
    dentry = (struct dir_entry*)bh->b_data;
    for (iCnt = 0; iCnt < file_size; iCnt += DIR_ENTRY_BYTES, dentry++) {
        printk("entry no %d, i_no: %u, entry_name: %s\n", 
                (int)(iCnt / DIR_ENTRY_BYTES), dentry->dir_inode,
                dentry->dir_name);
    }
    iput(inode);
    brelse(bh);

}

int create_root(void)
{
    printk("Creating ROOT directory\n");
    loff_t file_size = 0;

    struct buffer_head *bh;
    struct inode rinode;
    struct inode *inode;
    /* inode = iget(DEV_NO, 1); */
    /* if (!inode) { */
    /*     printk("failed to get inode 1\n"); */
    /*     return -1; */
    /* } */

    
    
    rinode.i_no = 1;
    read_inode(&rinode);
    inode = &rinode;

    bh = balloc(DEV_NO);
    if (!bh) {
        printk("failed to allocate a disk block\n");
        return -1;
    }
    inode->i_blocks[0] = bh->b_blocknr;

    char *root_names[] = {".", "..", "dev", "etc", "usr", "mnt", "bin", "boot", "home"};
    int iCnt = 0, root_list_size = sizeof(root_names) / sizeof(root_names[0]);
    for (iCnt = 0; iCnt < 2; iCnt++) {
        printk("creating directory : %s\n", root_names[iCnt]);
        if (strcmp(root_names[iCnt], "..") == 0 ||
                strcmp(root_names[iCnt], ".") == 0) {
            create_entry(root_names[iCnt], bh, iCnt * DIR_ENTRY_BYTES, (unsigned long)1);
        }
        /* else { */
        /*     struct inode *ientry = ialloc(DEV_NO); */
        /*     if (!ientry) { */
        /*         printk("ialloc failed in creating root directory\n"); */
        /*         return -1; */
        /*     } */
        /*     create_entry(root_names[iCnt], bh, iCnt * DIR_ENTRY_BYTES, ientry->i_no); */
        /*     iput(ientry); */
        /* } */
        file_size += DIR_ENTRY_BYTES;
    }
    inode->i_size = file_size;
    SET_FLAG(inode->i_state, I_DIRTY_INODE | I_DIRTY_DATA);
    write_inode(inode);
    /* iput(inode); */
    bwrite(bh);
    return 0;
}


void init_fs(void)
{
    int iCnt = 0;
    if (!super) {
        struct buffer_head *bh;
        bh = bread(DEV_NO, 1);
        if (!bh) {
            printk("read super blocked failed \n");
            return;
        }
        super = (s_ufs*)bh->b_data;
    }

    itable.table = (struct inode**)kzalloc(INCORE_TABLE, 0);
    if (!itable.table) {
        printk("initialization of incore table failed\n");
        return;
    }
    for (iCnt = 0; iCnt < INCORE_TABLE; iCnt++) {
        itable.table[iCnt] = NULL;
    }

    /*initiliaze file table */
    INIT_LIST_HEAD(&file_table);


    /* bwrite(bh); */
    /* never release super block */
    /* brelse(bh); */
}

int sys_open(const char *pathname, int flags, int mode)
{

    int fd;
    struct inode *inode;
    inode = namei(pathname);
    if (!inode) {
        printk("no such existing file %s\n", pathname);
        return -1;
    }
    fd = allocate_file(inode);
    inode = unlocked_inode(inode);
    return fd;
}


