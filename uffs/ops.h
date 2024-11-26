#ifndef OPS_H
#define OPS_H

#include <fuse.h>

int uffs_read(const char *path, char *buf, size_t size, off_t offset
                             , struct fuse_file_info *fi);
int uffs_getattr(const char *path, struct stat *stbuf);
int uffs_open(const char *path, struct fuse_file_info *fi);

#endif
