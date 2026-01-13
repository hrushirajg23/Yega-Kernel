#ifndef _NAMEI_H
#define _NAMEI_H

#include "vfs.h"

/* Flags for path lookup */
#define LOOKUP_FOLLOW    0x0001  /* follow symbolic links */
#define LOOKUP_DIRECTORY 0x0002  /* must be a directory */
#define LOOKUP_PARENT    0x0004  /* return parent directory */

/* Path lookup context */
struct nameidata {
    struct dentry *dentry;      /* current dentry */
    struct vfsmount *mnt;       /* current vfsmount */
    unsigned int flags;         /* lookup flags */
    int last_type;              /* type of last component */
    char *last_name;            /* last component name */
};

/* Path lookup functions */
int path_lookup(const char *path, unsigned int flags, struct nameidata *nd);
int link_path_walk(const char *path, struct nameidata *nd);
struct dentry *lookup_one_len(const char *name, struct dentry *base, int len);
int path_walk(const char *path, struct nameidata *nd);

/* Helper functions */
void path_release(struct nameidata *nd);
int follow_down(struct vfsmount **mnt, struct dentry **dentry);
int follow_up(struct vfsmount **mnt, struct dentry **dentry);

#endif /* _NAMEI_H */
