#include "ops.h"

#include "uffs.h"
#include "ops.h"
#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define BOOT_SECTOR_SIZE            0x400

static struct uffs_DeviceSt deviceSt;

// FUSE의 init 핸들러
void *uffs_init(struct fuse_conn_info *info) {
    if (device_fill() != 0) {
        printf("uffs cannot continue\n");
        abort();
    }
}

int device_fill(void)
{
    disk_read(BOOT_SECTOR_SIZE, sizeof(struct uffs_DeviceSt), &deviceSt);
    return 0;
}

int disk_read(off_t where, size_t size, void *p, const char *func, int line)
{
    static pthread_mutex_t read_lock = PTHREAD_MUTEX_INITIALIZER;
    ssize_t pread_ret;

    pthread_mutex_lock(&read_lock);
    pread_ret = pread_wrapper(disk_fd, p, size, where);
    pthread_mutex_unlock(&read_lock);

    return pread_ret;
}

static int pread_wrapper(int disk_fd, void *p, size_t size, off_t where)
{
    return pread(disk_fd, p, size, where);
}
