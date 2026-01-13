#include "fs.h"
#include "vfs.h"
#include "serial.h"
#include "string.h"
#include "slab.h"

/* External operation tables */
extern struct inode_operations ext2_dir_inode_operations;
extern struct inode_operations ext2_file_inode_operations;
extern struct file_operations ext2_file_operations;

/* Allocate a new ext2 inode */
struct inode *ext2_new_inode(struct inode *dir, int mode)
{
    uint32_t free_inode_n;
    struct inode *inode;
    
    /* Allocate inode number */
    if (reserve_free_inode(&free_inode_n) != 0) {
        printk("ext2_new_inode: no free inodes\n");
        return NULL;
    }
    
    /* Get VFS inode structure */
    inode = iget(dir ? dir->i_dev : DEV_NO, free_inode_n);
    if (!inode) {
        printk("ext2_new_inode: failed to get inode\n");
        return NULL;
    }
    
    /* Initialize inode */
    inode->i_mode = mode;
    inode->i_size = 0;
    inode->i_blocks = 0;
    inode->i_nlinks = 1;
    inode->i_uid = 0;
    inode->i_gid = 0;
    
    /* Set operations based on mode */
    if (mode & EXT2_S_IFDIR) {
        inode->i_op = &ext2_dir_inode_operations;
    } else {
        inode->i_op = &ext2_file_inode_operations;
        inode->f_op = &ext2_file_operations;
    }
    
    return inode;
}

/* Free an ext2 inode */
void ext2_free_inode(struct inode *inode)
{
    if (!inode) {
        return;
    }
    
    /* Free inode bitmap */
    unset_inode_bitmap(inode->i_no);
    
    /* TODO: Free data blocks */
    
    printk("ext2_free_inode: freed inode %d\n", inode->i_no);
}

/* Read inode from disk */
void ext2_read_inode(struct inode *inode)
{
    inode_t disk_inode;
    
    if (!inode) {
        return;
    }
    
    /* Read from disk */
    disk_inode = get_inode(inode->i_no);
    
    /* Copy to VFS inode */
    inode->i_mode = disk_inode.i_mode;
    inode->i_uid = disk_inode.i_uid;
    inode->i_gid = disk_inode.i_gid;
    inode->i_size = disk_inode.i_size;
    inode->i_atime = disk_inode.i_atime;
    inode->i_mtime = disk_inode.i_mtime;
    inode->i_ctime = disk_inode.i_ctime;
    inode->i_blocks = disk_inode.i_blocks;
    inode->i_nlinks = disk_inode.i_links_count;
    
    /* Set operations based on file type */
    if (disk_inode.i_mode & EXT2_S_IFDIR) {
        inode->i_op = &ext2_dir_inode_operations;
    } else {
        inode->i_op = &ext2_file_inode_operations;
        inode->f_op = &ext2_file_operations;
    }
}

/* Write inode to disk */
int ext2_write_inode(struct inode *inode)
{
    inode_t disk_inode;
    
    if (!inode) {
        return -1;
    }
    
    /* Read current inode from disk */
    disk_inode = get_inode(inode->i_no);
    
    /* Update fields */
    disk_inode.i_mode = inode->i_mode;
    disk_inode.i_uid = inode->i_uid;
    disk_inode.i_gid = inode->i_gid;
    disk_inode.i_size = inode->i_size;
    disk_inode.i_atime = inode->i_atime;
    disk_inode.i_mtime = inode->i_mtime;
    disk_inode.i_ctime = inode->i_ctime;
    disk_inode.i_blocks = inode->i_blocks;
    disk_inode.i_links_count = inode->i_nlinks;
    
    /* Write back to disk */
    write_inode_table(inode->i_no, disk_inode);
    
    return 0;
}

/* Delete inode */
void ext2_delete_inode(struct inode *inode)
{
    if (!inode) {
        return;
    }
    
    /* Free data blocks */
    /* TODO: Implement proper block freeing */
    
    /* Free inode */
    ext2_free_inode(inode);
}

/* Get block number for file offset */
int ext2_get_block(struct inode *inode, uint32_t block_idx, uint32_t *block_num)
{
    inode_t disk_inode;
    
    if (!inode || !block_num) {
        return -1;
    }
    
    /* Read inode from disk */
    disk_inode = get_inode(inode->i_no);
    
    /* Get block number */
    *block_num = get_data_block_n(disk_inode, block_idx);
    
    return 0;
}
