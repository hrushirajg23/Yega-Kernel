#ifndef _FILE_H
#define _FILE_H

#include "vfs.h"

/* Maximum number of open files per process */
#define MAX_FILES 256

/* File descriptor flags */
#define FD_CLOEXEC 1

/* File status flags */
#define O_RDONLY    0x0000
#define O_WRONLY    0x0001
#define O_RDWR      0x0002
#define O_CREAT     0x0100
#define O_TRUNC     0x0200
#define O_APPEND    0x0400
#define O_DIRECTORY 0x1000

/* Process file descriptor table */
struct files_struct {
    struct file *fd_array[MAX_FILES];
    int next_fd;
    unsigned int count;  /* reference count */
};

/* File descriptor operations */
int get_unused_fd(struct files_struct *files);
void fd_install(struct files_struct *files, int fd, struct file *file);
struct file *fget(struct files_struct *files, int fd);
void fput(struct file *file);
struct file *get_empty_filp(void);

/* File table initialization */
struct files_struct *init_files(void);
void destroy_files(struct files_struct *files);

/* File operations helpers */
int generic_file_open(struct inode *inode, struct file *filp);
int generic_file_release(struct inode *inode, struct file *filp);

#endif /* _FILE_H */
