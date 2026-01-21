#include "fs.h"
#include "vfs.h"
#include "dcache.h"
#include "serial.h"
#include "string.h"
#include "slab.h"

/* Forward declarations */
static struct dentry *ext2_lookup(struct inode *dir, struct dentry *dentry);
static int ext2_create(struct inode *dir, struct dentry *dentry, int mode);
static int ext2_mkdir(struct inode *dir, struct dentry *dentry, int mode);
static int ext2_rmdir(struct inode *dir, struct dentry *dentry);
static int ext2_rename(struct inode *old_dir, struct dentry *old_dentry,
                       struct inode *new_dir, struct dentry *new_dentry);

/* ext2 directory inode operations */
struct inode_operations ext2_dir_inode_operations = {
    .lookup = ext2_lookup,
    .create = ext2_create,
    .mkdir = ext2_mkdir,
    .rmdir = ext2_rmdir,
    .rename = ext2_rename,
    .link = NULL,
    .follow_link = NULL,
    .put_link = NULL,
    .permission = NULL,
    .truncate = NULL,
    .truncate_range = NULL,
};

/* ext2 file inode operations */
struct inode_operations ext2_file_inode_operations = {
    .lookup = NULL,
    .create = NULL,
    .mkdir = NULL,
    .rmdir = NULL,
    .rename = NULL,
    .link = NULL,
    .follow_link = NULL,
    .put_link = NULL,
    .permission = NULL,
    .truncate = NULL,
    .truncate_range = NULL,
};

/* Lookup directory entry */
static struct dentry *ext2_lookup(struct inode *dir, struct dentry *dentry)
{
    uint32_t block_len;
    uint32_t dentries_per_blk;
    ext2_file_handle dir_file;
    
    if (!dir || !dentry) {
        return NULL;
    }
    
    /* Convert VFS inode to ext2_file_handle for existing code */
    dir_file.inode = *(inode_t*)dir;  /* Cast VFS inode to ext2 inode */
    dir_file.inode_n = dir->i_no;
    
    block_len = i_block_len(dir_file.inode);
    dentries_per_blk = EXT2_BLK_SIZE / sizeof(dentry_t);
    
    /* Search through directory blocks */
    for (uint32_t i = 0; i < block_len; i++) {
        uint8_t *block = get_file_block(dir_file, i);
        if (!block) {
            continue;
        }
        
        dentry_t *entries = (dentry_t *)block;
        
        for (uint32_t j = 0; j < dentries_per_blk; j++) {
            dentry_t *entry = &entries[j];
            
            if (entry->inode == 0) {
                continue;  /* Empty entry */
            }
            
            if (!strcmp(entry->name, dentry->d_name)) {
                /* Found matching entry */
                inode_t disk_inode = get_inode(entry->inode);
                
                /* Create VFS inode */
                struct inode *inode = iget(dir->i_dev, entry->inode);
                if (!inode) {
                    kfree(block);
                    return NULL;
                }
                
                /* Copy ext2 inode data to VFS inode */
                inode->i_mode = disk_inode.i_mode;
                inode->i_size = disk_inode.i_size;
                inode->i_blocks = disk_inode.i_blocks;
                
                /* Set operations based on file type */
                if (disk_inode.i_mode & EXT2_S_IFDIR) {
                    inode->i_op = &ext2_dir_inode_operations;
                } else {
                    inode->i_op = &ext2_file_inode_operations;
                }
                
                /* Instantiate dentry with inode */
                d_instantiate(dentry, inode);
                
                kfree(block);
                return dentry;
            }
        }
        
        kfree(block);
    }
    
    /* Not found */
    return NULL;
}

/* Create regular file */
static int ext2_create(struct inode *dir, struct dentry *dentry, int mode)
{
    uint32_t free_inode_n, free_block_n;
    inode_t new_inode;
    dentry_t new_dentry;
    ext2_file_handle dir_file;
    
    if (!dir || !dentry) {
        return -1;
    }
    
    /* Allocate inode and block */
    if (reserve_free_inode(&free_inode_n) != 0) {
        printk("ext2_create: no free inodes\n");
        return -1;
    }
    
    if (reserve_free_block(&free_block_n) != 0) {
        printk("ext2_create: no free blocks\n");
        return -1;
    }
    
    /* Create new inode */
    memset(&new_inode, 0, sizeof(inode_t));
    new_inode.i_mode = EXT2_S_IFREG | mode;
    new_inode.i_size = 0;
    new_inode.i_blocks = 2;  /* 1 block = 2 sectors */
    new_inode.i_links_count = 1;
    new_inode.i_block[0] = free_block_n;
    
    /* Write inode to disk */
    write_inode_table(free_inode_n, new_inode);
    
    /* Create directory entry */
    new_dentry.inode = free_inode_n;
    new_dentry.rec_len = sizeof(dentry_t);
    new_dentry.name_len = strlen(dentry->d_name);
    new_dentry.file_type = EXT2_FT_REG_FILE;
    strncpy(new_dentry.name, dentry->d_name, sizeof(new_dentry.name) - 1);
    
    /* Add to parent directory */
    dir_file.inode = *(inode_t*)dir;
    dir_file.inode_n = dir->i_no;
    
    if (add_dentry(dir_file, new_dentry) != 0) {
        printk("ext2_create: failed to add dentry\n");
        return -1;
    }
    
    /* Create VFS inode and instantiate dentry */
    struct inode *vfs_inode = iget(dir->i_dev, free_inode_n);
    if (vfs_inode) {
        vfs_inode->i_mode = new_inode.i_mode;
        vfs_inode->i_size = new_inode.i_size;
        vfs_inode->i_blocks = new_inode.i_blocks;
        vfs_inode->i_op = &ext2_file_inode_operations;
        
        d_instantiate(dentry, vfs_inode);
    }
    
    return 0;
}

/* Create directory */
static int ext2_mkdir(struct inode *dir, struct dentry *dentry, int mode)
{
    uint32_t free_inode_n, free_block_n;
    inode_t new_inode;
    dentry_t new_dentry;
    ext2_file_handle dir_file;
    
    if (!dir || !dentry) {
        return -1;
    }
    
    /* Allocate inode and block */
    if (reserve_free_inode(&free_inode_n) != 0) {
        printk("ext2_mkdir: no free inodes\n");
        return -1;
    }
    
    if (reserve_free_block(&free_block_n) != 0) {
        printk("ext2_mkdir: no free blocks\n");
        return -1;
    }
    
    /* Create new directory inode */
    new_inode = new_dir_inode();
    new_inode.i_mode |= mode;
    new_inode.i_blocks = 2;
    new_inode.i_block[0] = free_block_n;
    
    /* Write inode to disk */
    write_inode_table(free_inode_n, new_inode);
    
    /* Create directory entry */
    new_dentry.inode = free_inode_n;
    new_dentry.rec_len = sizeof(dentry_t);
    new_dentry.name_len = strlen(dentry->d_name);
    new_dentry.file_type = EXT2_FT_DIR;
    strncpy(new_dentry.name, dentry->d_name, sizeof(new_dentry.name) - 1);
    
    /* Add to parent directory */
    dir_file.inode = *(inode_t*)dir;
    dir_file.inode_n = dir->i_no;
    
    if (add_dentry(dir_file, new_dentry) != 0) {
        printk("ext2_mkdir: failed to add dentry\n");
        return -1;
    }
    
    /* Create VFS inode and instantiate dentry */
    struct inode *vfs_inode = iget(dir->i_dev, free_inode_n);
    if (vfs_inode) {
        vfs_inode->i_mode = new_inode.i_mode;
        vfs_inode->i_size = new_inode.i_size;
        vfs_inode->i_blocks = new_inode.i_blocks;
        vfs_inode->i_op = &ext2_dir_inode_operations;
        
        d_instantiate(dentry, vfs_inode);
    }
    
    return 0;
}

/* Remove directory */
static int ext2_rmdir(struct inode *dir, struct dentry *dentry)
{
    /* Simplified implementation - just mark inode as free */
    if (!dir || !dentry || !dentry->d_inode) {
        return -1;
    }
    
    uint32_t inode_num = dentry->d_inode->i_no;
    
    /* Free inode bitmap */
    unset_inode_bitmap(inode_num);
    
    /* TODO: Free data blocks */
    /* TODO: Remove from parent directory */
    
    printk("ext2_rmdir: removed directory inode %d\n", inode_num);
    
    return 0;
}

/* Rename file/directory */
static int ext2_rename(struct inode *old_dir, struct dentry *old_dentry,
                       struct inode *new_dir, struct dentry *new_dentry)
{
    /* Simplified implementation */
    if (!old_dir || !old_dentry || !new_dir || !new_dentry) {
        return -1;
    }
    
    /* TODO: Implement proper rename */
    printk("ext2_rename: not fully implemented\n");
    
    return -1;
}
