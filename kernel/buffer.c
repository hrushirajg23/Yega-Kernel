#include "slab.h"
#include "mm.h"
#include "fs.h"
#include "serial.h"
#include "disk.h"

#define BUFFERS 20
#define BUFFER_SIZE 1024
kmem_cache_t *bcache;
struct buffer_cache buffer_cache;

#define hash_fn(bno, devno) (bno % devno)
struct buffer_head *search_hash(unsigned short dev_no, unsigned long blocknr);

void test_bcache(void);

void binit(struct buffer_head *bhead, unsigned short b_dev, unsigned long blocknr)
{
    bhead->flags = 0;
    bhead->b_data = kmalloc(BUFFER_SIZE, 0);
    if (!bhead->b_data) {
        printk("failed buffer_head kmalloc\n");
        return;
    }
    bhead->b_dev = b_dev;
    bhead->b_blocknr = blocknr;
    bhead->b_count = 0;
    INIT_LIST_NULL(&bhead->b_hash);
    INIT_LIST_NULL(&bhead->b_free);
}

void display_buffer_cache(void) 
{
    int iCnt = 0;
    for (; iCnt < DEV_NO; iCnt++) {
        struct list_head *run;
        struct buffer_head *bh;
        printk("================ b_hash %d=================\n", iCnt);
        list_for_each(run, &buffer_cache.b_hash[iCnt]) {
            bh = list_entry(run, struct buffer_head, b_hash);
            printk("<= %d => ", bh->b_blocknr);
        }
        printk("\n");
    }
}

void create_buffer_cache(void) 
{
    bcache = kmem_cache_create("buffer_head", sizeof(struct buffer_head), KMALLOC_MINALIGN, SLAB_HWCACHE_ALIGN, NULL);
    if (!bcache) {
        printk("failed kmem_cache_create for buffer_head\n");
        return;
    }


    int iCnt = 0;
    for (; iCnt < DEV_NO; iCnt++) {
        INIT_LIST_HEAD(&buffer_cache.b_hash[iCnt]);
    }
    INIT_LIST_HEAD(&buffer_cache.b_free);

    struct buffer_head *tmp = NULL;
    for (iCnt = 0; iCnt < BUFFERS; iCnt++) {
        tmp = kmem_cache_alloc(bcache, 0); 
        if (!tmp) {
            printk("failed allocating buffer_head for %d\n", iCnt);
            break;
        }
       
        binit(tmp, DEV_NO, iCnt);
        /*
         * add to correct hash list and also to freelist */
        list_add(&buffer_cache.b_hash[hash_fn(iCnt, DEV_NO)], &tmp->b_hash);
        list_add(&buffer_cache.b_free, &tmp->b_free);
    }
    printk("size of buffer_head is %d\n", sizeof(struct buffer_head));

    printk("displaying buffer cache................\n");
    display_buffer_cache();
    test_bcache();

}

struct buffer_head *search_hash(unsigned short dev_no, unsigned long blocknr)
{
    struct buffer_head *bh = NULL;
    struct list_head *run;
    int index = hash_fn(blocknr, dev_no);

    list_for_each(run, &buffer_cache.b_hash[index]) {
        bh = list_entry(run, struct buffer_head, b_hash);
        if (bh->b_blocknr == blocknr && bh->b_dev == dev_no) {
            return bh;
        }
    }
    return NULL;
}

static inline struct buffer_head *locked_buffer(struct buffer_head *bh)
{
    SET_FLAG(bh->flags, BH_lock);
    return bh;
}

static inline struct buffer_head *unlocked_buffer(struct buffer_head *bh)
{
    CLEAR_FLAG(bh->flags, BH_lock);
    return bh;
}

struct buffer_head *getblk(unsigned short dev_no, unsigned long blocknr)
{
    struct buffer_head *bh = NULL;
    struct list_head *run = NULL;
    printk("inside getblk \n");
    
    while (1) {
        bh = search_hash(dev_no, blocknr);
        if (bh) {
            printk("getblk hash found \n");
            if (IS_FLAG(bh->flags, BH_lock)) { //scenario 5
                //sleep
                printk("getblk scenario 5\n");
                continue;
            }
            printk("getblk scenario 1\n");
            SET_FLAG(bh->flags, BH_lock); //scenario 1
            list_del(&bh->b_free); //remove from the free list
            return locked_buffer(bh);
        }
        else { //block is not on hash queue
            if (list_is_empty(&buffer_cache.b_free)) { //scenario 4
                //sleep till any buffer doesn't become free
                printk("getblk scenario 4\n");
                continue; //to avoid race conditions 
            }
            /*
             * remove the first free buffer from free list */
            bh = list_first_entry(run, struct buffer_head, b_free);
            list_del(&bh->b_free);

            if (IS_FLAG(bh->flags, BH_delay)) { //scenario 3 marked for delayed write
                //async write to disk
                continue; 
            }

            printk("getblk scenario 2\n");

            //scenario 2 -- found a free buffer, use it 
            //remove the buffer from the old hash queue
            list_del(&bh->b_hash);
            list_add(&buffer_cache.b_hash[hash_fn(dev_no, blocknr)], &bh->b_hash);
            return locked_buffer(bh);
        }
    }
    return NULL;
}

void brelse(struct buffer_head *bh)
{
    /*
     * 1 wakeup all procs: event, waiting for any buffer to become free
     * 2 wakeup all procs: event, waiting for this buffer to become free
     *
     * raise processor execution level to block interrupts
     *
     */
    asm volatile ("cli");

    if ( !IS_FLAG(bh->flags, BH_dirty) && !IS_FLAG(bh->flags, BH_old)) {
        list_add_tail(&buffer_cache.b_free, &bh->b_free);
    }
    else {
        list_add(&buffer_cache.b_free, &bh->b_free);
    }

    asm volatile ("sti");
    unlocked_buffer(bh);
}

struct buffer_head *bread(unsigned short dev_no, unsigned long blocknr)
{
    printk("invoed bread \n");
    struct buffer_head *bh = getblk(dev_no, blocknr);
    //check is buffer is valid
    
    printk("invoed bread 2 \n");
    if (!IS_FLAG(bh->flags, BH_dirty)) {
        return bh;
    }

    printk("invoed bread 3 \n");
    /*
     * initiate disk read and sleep till then   
     */
    printk("invoking disk_read inside bread\n");
    disk_read(bh->b_blocknr, bh->b_data);
    printk("invoking disk_read compledted bread\n");
    return bh;
}


void bwrite(struct buffer_head *bh)
{
    disk_write(bh->b_blocknr, bh->b_data, BUFFER_SIZE);
    /*
     * if I/O is synchronous 
     *      sleep(event I/O completes)
     *      brelse
     * else if buffer is marked for delayed write
     *      mark buffer to put at head of list 
     *      
     *      I guess this is where its marked old
     *
     */
    brelse(bh);
}

void test_bcache(void)
{
    printk("testing buffer cache .............\n");
    struct buffer_head *bh = getblk(DEV_NO, 16);
    if (!bh) {
        printk(" getblk failed \n");
        return ;
    }

    
    for (int iCnt = 0; iCnt < BUFFER_SIZE; iCnt++ )
    {
        bh->b_data[iCnt] = iCnt / 256 + 'a';
    }
    printk("invoking bwrite\n");

    bwrite(bh);
    
    printk("bwrite completed\n");

    unsigned char *ptr = (unsigned char*)kmalloc(BUFFER_SIZE, 0);
    if (!ptr) {
        printk("kmalloc failed in testing bcache\n");
        return;
    }
    struct buffer_head *tmp = NULL;

    printk("invoking bread\n");
    tmp = bread(DEV_NO, 16); 

    printk("bread completed\n");
    printk(" buffer content %s\n", tmp->b_data);

}
