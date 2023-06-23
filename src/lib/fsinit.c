#include <fsinit.h>
#include <tmpfs.h>
#include <mm.h>
#include <panic.h>
#include <uartfs.h>

void fs_early_init(void){
    struct filesystem *tmpfs, *cpiofs, *uartfs;

    vfs_init();
    tmpfs = tmpfs_init();
    cpiofs = cpiofs_init();
    uartfs = uartfs_init();
    register_filesystem(tmpfs);
    register_filesystem(cpiofs);
    register_filesystem(uartfs);

    vfs_init_rootmount(tmpfs);

    vfs_mkdir("/initramfs");
    vfs_mount("/initramfs", "cpiofs");

    vfs_mkdir("/dev");

    vfs_mkdir("/dev/uart");
    vfs_mount("/dev/uart", "uartfs");
}