#include "namei.h"
#include "dcache.h"
#include "string.h"
#include "serial.h"
#include "string.h"

/* Skip leading slashes */
static const char *skip_slashes(const char *path)
{
    while (*path == '/') {
        path++;
    }
    return path;
}

/* Get next path component */
static const char *get_component(const char *path, char *component, int max_len)
{
    int len = 0;
    
    /* Skip leading slashes */
    path = skip_slashes(path);
    
    /* Copy component until slash or end */
    while (*path && *path != '/' && len < max_len - 1) {
        component[len++] = *path++;
    }
    component[len] = '\0';
    
    return path;
}

/* Lookup single component in directory */
static struct dentry *do_lookup(struct dentry *parent, const char *name)
{
    struct dentry *dentry;
    struct inode *dir;
    
    if (!parent || !name || !name[0]) {
        return NULL;
    }
    
    dir = parent->d_inode;
    if (!dir) {
        return NULL;
    }
    
    /* Check dentry cache first */
    dentry = d_lookup(parent, name);
    if (dentry) {
        return dentry;
    }
    
    /* Allocate new dentry */
    dentry = d_alloc(parent, name);
    if (!dentry) {
        return NULL;
    }
    
    /* Call filesystem's lookup operation */
    if (dir->i_op && dir->i_op->lookup) {
        struct dentry *result = dir->i_op->lookup(dir, dentry);
        if (!result) {
            /* Lookup failed, free dentry */
            dput(dentry);
            return NULL;
        }
        /* Lookup succeeded, dentry is now populated */
        return result;
    }
    
    /* No lookup operation */
    dput(dentry);
    return NULL;
}

/* Walk path components */
int link_path_walk(const char *path, struct nameidata *nd)
{
    char component[64];
    struct dentry *dentry;
    
    if (!path || !nd || !nd->dentry) {
        return -1;
    }
    
    /* Start from current dentry */
    dentry = nd->dentry;
    
    /* Skip leading slashes */
    path = skip_slashes(path);
    
    /* Walk through path components */
    while (*path) {
        /* Get next component */
        path = get_component(path, component, sizeof(component));
        
        if (!component[0]) {
            break;
        }
        
        /* Handle "." */
        if (!strcmp(component, ".")) {
            continue;
        }
        
        /* Handle ".." */
        if (!strcmp(component, "..")) {
            if (dentry->d_parent != dentry) {
                dentry = dentry->d_parent;
            }
            continue;
        }
        
        /* Lookup component */
        dentry = do_lookup(dentry, component);
        if (!dentry) {
            return -1;  /* Component not found */
        }
    }
    
    /* Update nameidata */
    nd->dentry = dentry;
    
    return 0;
}

/* Main path lookup function */
int path_lookup(const char *path, unsigned int flags, struct nameidata *nd)
{
    if (!path || !nd) {
        return -1;
    }
    
    /* Initialize nameidata */
    nd->flags = flags;
    nd->last_type = 0;
    nd->last_name = NULL;
    
    /* For now, we don't have a proper root, so this is simplified */
    /* In a real implementation, we'd get the root from current process */
    if (!nd->dentry) {
        printk("path_lookup: no root dentry\n");
        return -1;
    }
    
    /* Absolute path starts from root */
    if (path[0] == '/') {
        /* Walk up to root */
        while (nd->dentry->d_parent != nd->dentry) {
            nd->dentry = nd->dentry->d_parent;
        }
    }
    
    /* Walk the path */
    return link_path_walk(path, nd);
}

/* Lookup one component */
struct dentry *lookup_one_len(const char *name, struct dentry *base, int len)
{
    char component[64];
    
    if (!name || !base || len <= 0 || len >= sizeof(component)) {
        return NULL;
    }
    
    /* Copy name */
    strncpy(component, name, len);
    component[len] = '\0';
    
    return do_lookup(base, component);
}

/* Release path */
void path_release(struct nameidata *nd)
{
    if (!nd) {
        return;
    }
    
    if (nd->dentry) {
        dput(nd->dentry);
        nd->dentry = NULL;
    }
}

/* Follow mount point down */
int follow_down(struct vfsmount **mnt, struct dentry **dentry)
{
    /* Simplified - no mount support yet */
    return 0;
}

/* Follow mount point up */
int follow_up(struct vfsmount **mnt, struct dentry **dentry)
{
    /* Simplified - no mount support yet */
    return 0;
}

/* Simple path walk wrapper */
int path_walk(const char *path, struct nameidata *nd)
{
    return link_path_walk(path, nd);
}
