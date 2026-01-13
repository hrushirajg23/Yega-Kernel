#ifndef _DCACHE_H
#define _DCACHE_H

#include "vfs.h"
#include "list.h"

/* Dentry cache hash table size */
#define DENTRY_HASH_SIZE 256

/* Dentry flags */
#define DCACHE_REFERENCED 0x0001  /* recently used */
#define DCACHE_UNHASHED   0x0002  /* not in hash table */

/* Dentry cache operations */
struct dentry *d_alloc(struct dentry *parent, const char *name);
struct dentry *d_alloc_root(struct inode *root_inode);
void d_instantiate(struct dentry *dentry, struct inode *inode);
struct dentry *d_lookup(struct dentry *parent, const char *name);
void d_add(struct dentry *dentry, struct inode *inode);
void d_delete(struct dentry *dentry);
void dput(struct dentry *dentry);
struct dentry *dget(struct dentry *dentry);

/* Dentry cache initialization */
void dcache_init(void);

/* Helper functions */
int d_unhashed(struct dentry *dentry);
void d_rehash(struct dentry *dentry);
void d_move(struct dentry *dentry, struct dentry *target);

#endif /* _DCACHE_H */
