#include <vfs.h>
#include <list.h>
#include <sched.h>
#include <current.h>
#include <string.h>
#include <panic.h>
#include <mm.h>

struct mount *rootmount;

static struct list_head filesystems;

static struct vnode *get_dir_vnode(struct vnode *dir_node, const char **pathname){
    struct vnode *result;
    const char *start;
    const char *end;
    char buf[0x100];

    start = end = *pathname;

    if(*start == '/')
        result = rootmount->root;
    else
        result = dir_node;

    while(1){
        if(!strncmp("./", start, 2)){
            start += 2;
            end = start;
            continue;
        }
        else if(!strncmp("../", start, 3)){
            if(result->parent){
                result = result->parent;
            }

            start += 3;
            end = start;
            continue;
        }
        while(*end != '\0' && *end != '/')
            end++;
        if(*end == '/'){
            int ret;
            if(start == end){
                end++;
                start = end;
                continue;
            }

            // TODO: Check if the length is less than 0x100
            memncpy(buf, start, end - start);
            buf[end - start] = 0;
            ret = result->v_ops->lookup(result, &result, buf);
            if(ret < 0)
                return NULL;
            end++;
            start = end;
        }
        else{
            break;
        }
    }
    *pathname = *start ? start : NULL;
    return result;
}

static struct filesystem *find_filesystem(const char *filesystem){
    struct filesystem *fs;
    list_for_each_entry(fs, &filesystems, fs_list){
        if(!strcmp(fs->name, filesystem))
            return fs;
    }
    return NULL;
}

void vfs_init(void){
    INIT_LIST_HEAD(&filesystems);
}

void vfs_init_rootmount(struct filesystem *fs){
    struct vnode *n;
    int ret;
    
    ret = fs->alloc_vnode(fs, &n);
    if(ret < 0)
        panic("vfs_init_rootmount failed");

    rootmount = kmalloc(sizeof(struct mount));
    rootmount->root = n;
    rootmount->fs = fs;
    n->mount = rootmount;
    n->parent = NULL;
}

int register_filesystem(struct filesystem *fs){
    list_add_tail(&fs->fs_list, &filesystems);
    return 0;
}

int vfs_open(const char *pathname, int flags, struct file *target){
    const char *curname;
    struct vnode *dir_node;
    struct vnode *file_node;
    int ret;
    curname = pathname;
    dir_node = get_dir_vnode(current->work_dir, &curname);
    if(!dir_node)
        return -1;
    if(!curname)
        return -1;
    ret = dir_node->v_ops->lookup(dir_node, &file_node, curname);
    if(flags & O_CREATE){
        if(ret == 0)
            return -1;

        ret = dir_node->v_ops->create(dir_node, &file_node, curname);
    }
    if(ret<0)
        return ret;
    if(!file_node)
        return -1;
    ret = file_node->f_ops->open(file_node, target);
    if(ret < 0)
        return ret;
    target->flags = 0;
    return 0;
}

int vfs_close(struct file *file){
    return file->f_ops->close(file);
}

int vfs_write(struct file *file, const void *buf, size_t len){
    return file->f_ops->write(file, buf, len);
}

int vfs_read(struct file *file, void *buf, size_t len){
    return file->f_ops->read(file, buf, len);
}

int vfs_mkdir(const char *pathname){
    const char *curname;
    struct vnode *dir_node;
    struct vnode *newdir_node;
    int ret;

    curname = pathname;

    if (current)
        dir_node = get_dir_vnode(current->work_dir, &curname);
    else 
        dir_node = get_dir_vnode(rootmount->root, &curname);

    if(!dir_node)
        return -1;
    if(!curname)
        return -1;

    ret = dir_node->v_ops->mkdir(dir_node, &newdir_node, curname);
    return ret;
}

int vfs_mount(const char *mountpath, const char *filesystem){
    const char *curname;
    struct vnode *dir_node;
    struct filesystem *fs;
    struct mount *mo;
    int ret;

    curname = mountpath;
    if (current)
        dir_node = get_dir_vnode(current->work_dir, &curname);
    else 
        dir_node = get_dir_vnode(rootmount->root, &curname);
    if(!dir_node)
        return -1;

    if(curname){
        ret = dir_node->v_ops->lookup(dir_node, &dir_node, curname);
        if(ret < 0)
            return ret;
    }

    if(!dir_node->v_ops->isdir(dir_node))
        return -1;
    fs = find_filesystem(filesystem);
    if(!fs)
        return -1;
    mo = kmalloc(sizeof(struct mount));
    mo->root = dir_node;
    mo->fs = fs;
    ret = fs->mount(fs, mo);

    if(ret < 0){
        kfree(mo);
        return ret;
    }

    return 0;
}

int vfs_lookup(const char *pathname, struct vnode **target){
    const char *curname;
    struct vnode *dir_node;
    struct vnode *file_node;
    int ret;
    curname = pathname;
    dir_node = get_dir_vnode(current->work_dir, &curname);
    if(!dir_node)
        return -1;
    if(!curname){
        *target = dir_node;
        return 0;
    }
    ret = dir_node->v_ops->lookup(dir_node, &file_node, curname);
    if(ret < 0)
        return ret;
    *target = file_node;
    return 0;
}

static int do_open(const char *pathname, int flags){
    int i, ret;
    for(i = 0 ; i <= current->maxfd ;++i){
        if(current->fds[i].vnode == NULL)
            break;
    }
    if(i > current->maxfd){
        if(current->maxfd >= TASK_MAX_FD){
            return -1;
        }
        current->maxfd += 1;
        i = current->maxfd;
    }
    ret = vfs_open(pathname, flags, &current->fds[i]);
    if(ret < 0)
        return ret;
    return i;
}

static int do_close(int fd){
    int ret;
    if(fd < 0 || current->maxfd < fd)
        return -1;
    if(current->fds[fd].vnode == NULL)
        return -1;
    ret = vfs_close(&current->fds[fd]);
    if(ret < 0)
        return ret;
    return 0;
}

static int do_write(int fd, const void *buf, uint64 count){
    int ret;
    if(fd < 0 || current->maxfd < fd)
        return -1;
    if(current->fds[fd].vnode == NULL)
        return -1;
    ret = vfs_write(&current->fds[fd], buf, count);
    return ret;
}

static int do_read(int fd, void *buf, uint64 count){
    int ret;
    if(fd < 0 || current->maxfd < fd)
        return -1;
    if(current->fds[fd].vnode == NULL)
        return -1;
    ret = vfs_read(&current->fds[fd], buf, count);
    return ret;
}

static int do_mkdir(const char *pathname, uint32 mode){
    int ret;
    ret = vfs_mkdir(pathname);
    return ret;
}

static int do_mount(const char *target, const char *filesystem){
    int ret;
    ret = vfs_mount(target, filesystem);
    return ret;
}

static int do_chdir(const char *path){
    struct vnode *result;
    int ret;

    ret = vfs_lookup(path, &result);
    if(ret < 0)
        return ret;
    if(!result->v_ops->isdir(result))
        return -1;
    current->work_dir = result;
    return 0;
}

void syscall_open(trapframe *frame, const char *pathname, int flags){
    int fd = do_open(pathname, flags);
    frame->x0 = fd;
}
void syscall_close(trapframe *frame, int fd){
    int ret = do_close(fd);
    frame->x0 = ret;
}
void syscall_write(trapframe *frame, int fd, const void *buf, uint64 count){
    // uart_sync_printf("count is %d\r\n", count);
    int ret = do_write(fd, buf, count);
    frame->x0 = ret;
}
void syscall_read(trapframe *frame, int fd, void *buf, uint64 count){
    int ret = do_read(fd, buf, count);
    frame->x0 = ret;
}
void syscall_mkdir(trapframe *frame, const char *pathname, uint32 mode){
    int ret = do_mkdir(pathname, mode);
    frame->x0 = ret;
}
void syscall_mount(trapframe *frame, const char *src, const char *target, const char *filesystem, uint64 flags, const void *data){
    int ret = do_mount(target, filesystem);
    frame->x0 = ret;
}
void syscall_chdir(trapframe *frame, const char *path){
    int ret = do_chdir(path);
    frame->x0 = ret;
}
void syscall_lseek64(trapframe *frame, int fd, int64 offset, int whence){
    // TODO
    int ret = -1;
    frame->x0 = ret;
}
void syscall_ioctl(trapframe *frame, int fd, uint64 request, ...){
    // TODO
    int ret = -1;
    frame->x0 = ret;
}