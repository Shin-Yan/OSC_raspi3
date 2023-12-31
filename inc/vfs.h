#ifndef _VFS_H
#define _VFS_H

#include <type.h>
#include <trapframe.h>
#include <list.h>
#include <stdarg.h>

#define O_CREATE        00000100

#define SEEK_SET        0
#define SEEK_CUR        1
#define SEEK_END        2

struct vnode {
    struct mount *mount;
    struct vnode_operations *v_ops;
    struct file_operations *f_ops;
    struct vnode *parent;
    void *internal;
};

struct mount {
    struct vnode *root;
    struct filesystem *fs;
};

struct filesystem {
    const char *name;
    /* Link all filesystems */
    struct list_head fs_list;
    int (*mount)(struct filesystem *fs, struct mount *mount);
    int (*alloc_vnode)(struct filesystem *fs, struct vnode **target);
    int (*sync)(struct filesystem *fs);
};

struct vnode_operations {
    int (*lookup)(struct vnode *dir_node, struct vnode **target,
                  const char *component_name);
    int (*create)(struct vnode *dir_node, struct vnode **target,
                  const char *component_name);
    int (*mkdir)(struct vnode *dir_node, struct vnode **target,
                const char *component_name);
    int (*isdir)(struct vnode *dir_node);
    int (*getname)(struct vnode *dir_node, const char **name);
    int (*getsize)(struct vnode *dir_node);
};

struct file {
    struct vnode *vnode;
    size_t f_pos;                    // RW position of this file handle
    struct file_operations *f_ops;
    int flags;
};

struct file_operations {
    int (*write)(struct file *file, const void *buf, size_t len);
    int (*read)(struct file *file, void *buf, size_t len);
    int (*open)(struct vnode *file_node, struct file *target);
    int (*close)(struct file *file);
    long (*lseek64)(struct file *file, long offset, int whence);
    int (*ioctl)(struct file *file, uint64 request, va_list args);
};

extern struct mount *rootmount;

void vfs_init(void);
void vfs_init_rootmount(struct filesystem *fs);

int register_filesystem(struct filesystem *fs);
int vfs_open(const char *pathname, int flags, struct file *target);
int vfs_close(struct file *target);
int vfs_write(struct file *file, const void *buf, size_t len);
int vfs_read(struct file *file, void *buf, size_t len);
int vfs_mkdir(const char *pathname);
int vfs_mount(const char *mountpath, const char *filesystem);
int vfs_lookup(const char *pathname, struct vnode **target);
int vfs_sync(struct filesystem *fs);
long vfs_lseek64(struct file *file, long offset, int whence);
int vfs_ioctl(struct file *file, uint64 request, va_list args);

void syscall_open(trapframe *frame, const char *pathname, int flags);
void syscall_close(trapframe *frame, int fd);
void syscall_write(trapframe *frame, int fd, const void *buf, uint64 count);
void syscall_read(trapframe *frame, int fd, void *buf, uint64 count);
void syscall_mkdir(trapframe *frame, const char *pathname, uint32 mode);
void syscall_mount(trapframe *frame, const char *src, const char *target, const char *filesystem, uint64 flags, const void *data);
void syscall_chdir(trapframe *frame, const char *path);
void syscall_lseek64(trapframe *frame, int fd, int64 offset, int whence);
void syscall_ioctl(trapframe *frame, int fd, uint64 request, ...);
void syscall_sync(trapframe *frame);

#endif