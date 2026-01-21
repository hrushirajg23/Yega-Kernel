#include "dcache.h"
#include "slab.h"
#include "string.h"
#include "serial.h"

/* Dentry cache hash table */
static struct list_head dentry_hashtable[DENTRY_HASH_SIZE];
static kmem_cache_t *dentry_cache;

/* Simple hash function for dentry cache */
static inline unsigned int d_hash(struct dentry *parent, const char *name)
{
    unsigned int hash = 0;
    const char *p = name;
    
    while (*p) {
        hash = hash * 31 + *p++;
    }
    
    if (parent) {
        hash ^= (unsigned int)parent;
    }
    
    return hash % DENTRY_HASH_SIZE;
}

/* Initialize dentry cache */
void dcache_init(void)
{
    int i;
    
    /* Create slab cache for dentries */
    dentry_cache = kmem_cache_create("dentry", sizeof(struct dentry), 
                                     KMALLOC_MINALIGN, SLAB_HWCACHE_ALIGN, NULL);
    if (!dentry_cache) {
        printk("Failed to create dentry cache\n");
        return;
    }
    
    /* Initialize hash table */
    for (i = 0; i < DENTRY_HASH_SIZE; i++) {
        INIT_LIST_HEAD(&dentry_hashtable[i]);
    }
    
    printk("Dentry cache initialized\n");
}

/* Allocate a new dentry */
struct dentry *d_alloc(struct dentry *parent, const char *name)
{
    struct dentry *dentry;
    
    dentry = (struct dentry *)kmem_cache_alloc(dentry_cache, 0);
    if (!dentry) {
        return NULL;
    }
    
    /* Initialize dentry */
    dentry->d_count = 1;
    dentry->d_flags = 0;
    dentry->d_inode = NULL;
    dentry->d_parent = parent ? parent : dentry;  /* root points to itself */
    dentry->d_op = NULL;
    dentry->sb = NULL;
    
    /* Copy name */
    if (name) {
        strncpy(dentry->d_name, name, sizeof(dentry->d_name) - 1);
        dentry->d_name[sizeof(dentry->d_name) - 1] = '\0';
    } else {
        dentry->d_name[0] = '\0';
    }
    
    /* Initialize lists */
    INIT_LIST_HEAD(&dentry->d_lru);
    INIT_LIST_HEAD(&dentry->d_child);
    INIT_LIST_HEAD(&dentry->d_subdirs);
    INIT_LIST_HEAD(&dentry->d_hash);
    
    /* Add to parent's subdirectory list */
    if (parent && parent != dentry) {
        list_add(&parent->d_subdirs, &dentry->d_child);
    }
    
    return dentry;
}

/* Allocate root dentry */
struct dentry *d_alloc_root(struct inode *root_inode)
{
    struct dentry *dentry;
    
    if (!root_inode) {
        return NULL;
    }
    
    dentry = d_alloc(NULL, "/");
    if (dentry) {
        dentry->d_inode = root_inode;
        dentry->sb = root_inode->i_sb;
        list_add(&root_inode->i_dentry, &dentry->d_child);
    }
    
    return dentry;
}

/* Connect dentry to inode */
void d_instantiate(struct dentry *dentry, struct inode *inode)
{
    if (!dentry || !inode) {
        return;
    }
    
    dentry->d_inode = inode;
    list_add(&inode->i_dentry, &dentry->d_child);
}

/* Lookup dentry in cache */
struct dentry *d_lookup(struct dentry *parent, const char *name)
{
    unsigned int hash;
    struct list_head *head, *tmp;
    struct dentry *dentry;
    
    if (!parent || !name) {
        return NULL;
    }
    
    hash = d_hash(parent, name);
    head = &dentry_hashtable[hash];
    
    list_for_each(tmp, head) {
        dentry = list_entry(tmp, struct dentry, d_hash);
        
        if (dentry->d_parent == parent && !strcmp(dentry->d_name, name)) {
            dentry->d_count++;
            return dentry;
        }
    }
    
    return NULL;
}

/* Add dentry to cache */
void d_add(struct dentry *dentry, struct inode *inode)
{
    unsigned int hash;
    
    if (!dentry) {
        return;
    }
    
    /* Instantiate with inode */
    if (inode) {
        d_instantiate(dentry, inode);
    }
    
    /* Add to hash table */
    hash = d_hash(dentry->d_parent, dentry->d_name);
    list_add(&dentry_hashtable[hash], &dentry->d_hash);
    dentry->d_flags &= ~DCACHE_UNHASHED;
}

/* Delete dentry from cache */
void d_delete(struct dentry *dentry)
{
    if (!dentry) {
        return;
    }
    
    /* Remove from hash table */
    if (!(dentry->d_flags & DCACHE_UNHASHED)) {
        list_del(&dentry->d_hash);
        dentry->d_flags |= DCACHE_UNHASHED;
    }
}

/* Release dentry reference */
void dput(struct dentry *dentry)
{
    if (!dentry) {
        return;
    }
    
    dentry->d_count--;
    
    if (dentry->d_count == 0) {
        /* Remove from parent's subdirectory list */
        if (dentry->d_parent != dentry) {
            list_del(&dentry->d_child);
        }
        
        /* Remove from hash table */
        d_delete(dentry);
        
        /* Free the dentry */
        kmem_cache_free(dentry_cache, dentry);
    }
}

/* Increment dentry reference */
struct dentry *dget(struct dentry *dentry)
{
    if (dentry) {
        dentry->d_count++;
    }
    return dentry;
}

/* Check if dentry is unhashed */
int d_unhashed(struct dentry *dentry)
{
    return dentry && (dentry->d_flags & DCACHE_UNHASHED);
}

/* Rehash a dentry */
void d_rehash(struct dentry *dentry)
{
    if (!dentry || !d_unhashed(dentry)) {
        return;
    }
    
    unsigned int hash = d_hash(dentry->d_parent, dentry->d_name);
    list_add(&dentry_hashtable[hash], &dentry->d_hash);
    dentry->d_flags &= ~DCACHE_UNHASHED;
}

/* Move dentry in the tree */
void d_move(struct dentry *dentry, struct dentry *target)
{
    if (!dentry || !target) {
        return;
    }
    
    /* Remove from old position */
    d_delete(dentry);
    
    /* Update parent */
    dentry->d_parent = target->d_parent;
    
    /* Add to new position */
    d_add(dentry, dentry->d_inode);
}
