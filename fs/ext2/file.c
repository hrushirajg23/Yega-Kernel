#include "file.h"
#include "slab.h"
#include "mm.h"
#include "serial.h"
#include "string.h"
#include "dcache.h"

/* Global file cache */
static kmem_cache_t *file_cache;

/* Initialize file cache */
static void file_cache_init(void)
{
    file_cache = kmem_cache_create("file", sizeof(struct file),
                                   KMALLOC_MINALIGN, SLAB_HWCACHE_ALIGN, NULL);
    if (!file_cache) {
        printk("Failed to create file cache\n");
    }
}

/* Initialize process file descriptor table */
struct files_struct *init_files(void)
{
    struct files_struct *files;
    int i;
    
    /* Initialize file cache if not done */
    if (!file_cache) {
        file_cache_init();
    }
    
    files = (struct files_struct *)kmalloc(sizeof(struct files_struct), 0);
    if (!files) {
        return NULL;
    }
    
    /* Initialize file descriptor array */
    for (i = 0; i < MAX_FILES; i++) {
        files->fd_array[i] = NULL;
    }
    
    files->next_fd = 0;
    files->count = 1;
    
    return files;
}

/* Destroy file descriptor table */
void destroy_files(struct files_struct *files)
{
    int i;
    
    if (!files) {
        return;
    }
    
    files->count--;
    
    if (files->count == 0) {
        /* Close all open files */
        for (i = 0; i < MAX_FILES; i++) {
            if (files->fd_array[i]) {
                fput(files->fd_array[i]);
            }
        }
        
        kfree(files);
    }
}

/* Get unused file descriptor */
int get_unused_fd(struct files_struct *files)
{
    int fd;
    
    if (!files) {
        return -1;
    }
    
    /* Find first free fd */
    for (fd = 0; fd < MAX_FILES; fd++) {
        if (files->fd_array[fd] == NULL) {
            return fd;
        }
    }
    
    return -1;  /* No free descriptors */
}

/* Install file in file descriptor table */
void fd_install(struct files_struct *files, int fd, struct file *file)
{
    if (!files || fd < 0 || fd >= MAX_FILES || !file) {
        return;
    }
    
    files->fd_array[fd] = file;
}

/* Get file from file descriptor */
struct file *fget(struct files_struct *files, int fd)
{
    struct file *file;
    
    if (!files || fd < 0 || fd >= MAX_FILES) {
        return NULL;
    }
    
    file = files->fd_array[fd];
    if (file) {
        file->f_count++;
    }
    
    return file;
}

/* Release file reference */
void fput(struct file *file)
{
    if (!file) {
        return;
    }
    
    file->f_count--;
    
    if (file->f_count == 0) {
        /* Call release operation if defined */
        if (file->f_op && file->f_op->release && file->f_dentry) {
            file->f_op->release(file->f_dentry->d_inode, file);
        }
        
        /* Release dentry reference */
        if (file->f_dentry) {
            dput(file->f_dentry);
        }
        
        /* Free file structure */
        kmem_cache_free(file_cache, file);
    }
}

/* Allocate empty file structure */
struct file *get_empty_filp(void)
{
    struct file *file;
    
    /* Initialize file cache if not done */
    if (!file_cache) {
        file_cache_init();
    }
    
    file = (struct file *)kmem_cache_alloc(file_cache, 0);
    if (!file) {
        return NULL;
    }
    
    /* Initialize file structure */
    INIT_LIST_HEAD(&file->f_list);
    file->f_dentry = NULL;
    file->f_op = NULL;
    file->f_count = 1;
    file->f_flags = 0;
    file->f_mode = 0;
    file->f_pos = 0;
    file->f_uid = 0;
    file->f_gid = 0;
    
    return file;
}

/* Generic file open */
int generic_file_open(struct inode *inode, struct file *filp)
{
    if (!inode || !filp) {
        return -1;
    }
    
    /* Set file operations from inode */
    filp->f_op = inode->f_op;
    
    return 0;
}

/* Generic file release */
int generic_file_release(struct inode *inode, struct file *filp)
{
    /* Nothing to do for generic release */
    return 0;
}
