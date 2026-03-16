/*
 * fuse_compat.h
 * 단위/통합 테스트 빌드용 최소 FUSE2 타입 정의.
 * libfuse-dev 없이도 컴파일 가능하도록 타입만 선언.
 */
#ifndef FUSE_COMPAT_H
#define FUSE_COMPAT_H

#include <sys/stat.h>
#include <sys/types.h>
#include <stdint.h>

struct fuse_file_info {
    int      flags;
    uint64_t fh;
};

/* FUSE2: 4인자 filler */
typedef int (*fuse_fill_dir_t)(void *buf, const char *name,
                               const struct stat *stbuf, off_t off);

struct fuse_operations {
    void *(*init)();
    int (*getattr) (const char *, struct stat *);
    int (*readdir) (const char *, void *, fuse_fill_dir_t, off_t,
                    struct fuse_file_info *);
    int (*opendir) (const char *, struct fuse_file_info *);
    int (*open)    (const char *, struct fuse_file_info *);
    int (*read)    (const char *, char *, size_t, off_t,
                    struct fuse_file_info *);
    int (*write)   (const char *, const char *, size_t, off_t,
                    struct fuse_file_info *);
    int (*create)  (const char *, mode_t, struct fuse_file_info *);
    int (*mkdir)   (const char *, mode_t);
};

static inline int fuse_main(int argc, char *argv[],
                             const struct fuse_operations *op, void *data)
{
    (void)argc; (void)argv; (void)op; (void)data;
    return 0;
}

#endif /* FUSE_COMPAT_H */
